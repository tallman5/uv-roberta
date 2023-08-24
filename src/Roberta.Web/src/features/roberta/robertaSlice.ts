import { createSelector, createSlice } from "@reduxjs/toolkit"
import { AppThunk, RootState } from "../../context"

export type GpsState = {
    heading: number
    isReady: boolean
    latitude: number
    longitude: number
    speed: number
}

const initialGpsState: GpsState = {
    heading: 0,
    isReady: false,
    latitude: 40.229057,
    longitude: -75.507144,
    speed: 0,
}

export type RobertaState = {
    gpsState: GpsState
}

const initialState: RobertaState = {
    gpsState: initialGpsState,
}

export const robertaSlice = createSlice({
    name: 'roberta',
    initialState,
    reducers: {
        updateGpsState: (state, action) => {
            return {
                ...state,
                gpsState: action.payload
            }
        }
    },
});

// Base selectors
export const selectNumber = (_: RootState, n: number) => n;
export const selectRobertaBase = (state: RootState) => state.roberta;

// Reselectors
export const selectRoberta = createSelector(selectRobertaBase, item => item);

// export const selectRoberta = createSelector(
//     selectPeopleBase,
//     list => list
// )
// export const selectPersonById = createSelector(
//     selectPeopleBase,
//     selectNumber,
//     (items, id) => {
//         return items.find(i => i.id === id)
//     }
// );

// Methods

export const { updateGpsState } = robertaSlice.actions;
export default robertaSlice.reducer;
