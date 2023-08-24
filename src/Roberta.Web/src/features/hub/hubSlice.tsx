import { createSelector, createSlice } from "@reduxjs/toolkit"
import { AppThunk, RootState } from "../../context"
const signalR = require("@microsoft/signalr")
import { HubConnection } from '@microsoft/signalr'
import { updateGpsState } from "../roberta/robertaSlice";
import { getAccessToken } from "../appUser/appUserSlice";
import { isBrowser } from "@tallman/strong-strap";

let hubConnection: HubConnection;

export type HubState = {
    connectionStatus: string
    hubUrl: string
}

const initialState: HubState = {
    connectionStatus: 'Disconnected',
    hubUrl: process.env.GATSBY_BASE_URL + "/robertaHub"
}

export const hubSlice = createSlice({
    name: 'hub',
    initialState,
    reducers: {
        setConnectionStatus: (state: HubState, action) => {
            state.connectionStatus = action.payload;
        },
    },
});

// Base selectors
export const selectConnectionStatusBase = (state: RootState) => state.hub.connectionStatus;

// Reselectors
export const selectConnectionStatus = createSelector(selectConnectionStatusBase, (status) => status)

// Methods
export const connectToHub = (): AppThunk => async (dispatch, getState) => {
    if (!isBrowser) return;

    if (hubConnection && (hubConnection.state === "Connected" || hubConnection.state === "Connecting"))
        return hubConnection;

    const state = getState()
    const currentStatus = state.hub.connectionStatus
    if (currentStatus !== 'Disconnected')
        return;

    const gat = dispatch(getAccessToken());
    const accessToken = (await gat).payload;
    if (accessToken == '') return;

    hubConnection = new signalR.HubConnectionBuilder()
        .withUrl(state.hub.hubUrl, { accessTokenFactory: () => accessToken })
        .withAutomaticReconnect()
        .build();

    // Connection events
    hubConnection.onreconnecting(() => { dispatch(setConnectionStatus(hubConnection.state)); })
    hubConnection.onreconnected(() => { dispatch(setConnectionStatus(hubConnection.state)); })
    hubConnection.onclose(() => { dispatch(setConnectionStatus(hubConnection.state)); })

    // Hub Events
    hubConnection.on("GpsStateUpdated", gpsState => { dispatch(updateGpsState(gpsState)); });

    const returnValue = await hubConnection.start()
        .then(() => {
            dispatch(setConnectionStatus(hubConnection.state));
            return hubConnection;
        })
        .catch((err) => {
            console.error(err)
            dispatch(setConnectionStatus(hubConnection.state));
            return null;
        });

    return returnValue;
};

export const disconnectHub = (): AppThunk => async () => {
    if (hubConnection && hubConnection.state !== "Disconnected")
        hubConnection.stop()
};

export const reconnectToHub = (): AppThunk => async (dispatch) => {
    dispatch(disconnectHub());
    dispatch(connectToHub());
};

export function getConnection() {
    return hubConnection
}

// Main Exports
export const { setConnectionStatus } = hubSlice.actions;
export default hubSlice.reducer;
