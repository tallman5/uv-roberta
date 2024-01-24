import { Action, configureStore, ThunkAction } from "@reduxjs/toolkit";
import React from "react";
import { Provider, TypedUseSelectorHook, useDispatch, useSelector } from "react-redux";
import appReducer from '../features/app/appSlice'
import appUserReducer from '../features/appUser/appUserSlice'
import hubReducer from '../features/hub/hubSlice'
import robertaReducer from '../features/roberta/robertaSlice'
import { msalConfig } from "./authConfig";
import { PublicClientApplication } from '@azure/msal-browser';
import { MsalProvider } from '@azure/msal-react';

export const msalInstance = new PublicClientApplication(msalConfig);

export const createStore = () => configureStore({
    reducer: {
        app: appReducer,
        appUser: appUserReducer,
        hub: hubReducer,
        roberta: robertaReducer,
    },
})

export const wrapWithProvider = ({ element }: any) => {
    const store = createStore();

    return (
        <Provider store={store}>
            <MsalProvider instance={msalInstance}>
                {element}
            </MsalProvider>
        </Provider>
    )
}

type ConfiguredStore = ReturnType<typeof createStore>
type StoreGetState = ConfiguredStore["getState"]
export type RootState = ReturnType<StoreGetState>
export type AppDispatch = ConfiguredStore["dispatch"]
export type AppThunk<ReturnType = void> = ThunkAction<
    ReturnType,
    RootState,
    unknown,
    Action<string>
>;
export const useAppDispatch: () => AppDispatch = useDispatch
export const useAppSelector: TypedUseSelectorHook<RootState> = useSelector
