import { createAsyncThunk, createSelector, createSlice } from '@reduxjs/toolkit';
import { RootState, AppThunk } from '../../context';
import { executeFetchAsync } from '@tallman/strong-strap';
import { msalInstance } from '../../context';
import { AuthenticationResult } from '@azure/msal-browser';
import jwtDecode from 'jwt-decode';
import { scopes } from '../../context/authConfig';

export type UserToken = {
    accessToken: string
    expires: Date
}

export type AppUserState = {
    accessToken: string | null,
    roles: string[],
}

export type WithRoles = {
    roles: string[],
}

const initialState: AppUserState = {
    accessToken: null,
    roles: [],
}

export const appUserSlice = createSlice({
    name: 'appUser',
    initialState: initialState,
    reducers: {
        clearToken: (state: AppUserState) => {
            state.accessToken = null;
            state.roles = [
            ]
        },
        setAccessToken: (state: AppUserState, action) => {
            state.accessToken = action.payload;
            const at = jwtDecode<WithRoles>(action.payload);
            if (at && at.roles) {
                state.roles = [
                    ...at.roles
                ]
            }
        },
        setRoles: (state: AppUserState, action) => {
            state.roles = [
                ...action.payload
            ]
        }
    },
});


// Base Selectors
export const selectAccessTokenBase = (state: RootState) => state.appUser.accessToken;
export const selectAppUserBase = (state: RootState) => state.appUser;


// Reselectors
export const selectAccessToken = createSelector(selectAccessTokenBase, (val: string | null) => val);
export const selectAppUser = createSelector(selectAppUserBase, (val: AppUserState) => val);


// Methods
export const getAccessToken = createAsyncThunk<string, void, { state: RootState }>(
    'getAccessToken',
    async (_, thunkAPI) => {
        let appUser = thunkAPI.getState().appUser;

        if (appUser?.accessToken) // need to check expiration
            return appUser.accessToken;

        const a = msalInstance.getActiveAccount();
        if (a) {
            const accessTokenRequest = {
                scopes,
                account: a,
            };
            msalInstance.acquireTokenSilent(accessTokenRequest)
                .then((accessTokenResponse) => {
                    thunkAPI.dispatch(setAccessToken(accessTokenResponse.accessToken));
                    return accessTokenResponse.accessToken;
                });
        }

        const guestUt: UserToken = await executeFetchAsync(process.env.GATSBY_BASE_URL + '/api/user/guestToken');
        if (guestUt && guestUt.accessToken) {
            thunkAPI.dispatch(setAccessToken(guestUt.accessToken));
            return guestUt.accessToken;
        }

        throw 'Could not aquire guest token.'
    }
);

// Main slice exports
export const { clearToken, setAccessToken } = appUserSlice.actions;
export default appUserSlice.reducer;
