import { createSelector, createSlice } from "@reduxjs/toolkit"
import { AppThunk, RootState } from "../../context"
import { UiFunction } from '@tallman/strong-strap';

export type Message = {
    uiFunction: UiFunction
    details?: string
    displayTimeout: number
    id: number
    title?: string
}

export type AppState = {
    messageList: Message[]
}

const initialState: AppState = {
    messageList: [],
}

export const appSlice = createSlice({
    name: 'app',
    initialState,
    reducers: {
        addMessage: (state: AppState, action) => {
            let newId = Math.max(...state.messageList.map(o => o.id), 0)
            newId++
            action.payload.id = newId
            state.messageList.push(action.payload)
        },
        clearMessages: (state) => {
            state.messageList = [];
        },
        deleteMessage: (state: AppState, action) => {
            state.messageList = state.messageList.filter(message => action.payload !== message.id)
        },
    },
});

// Base selectors
export const selectMessagesBase = (state: RootState) => state.app.messageList;

// Reselectors
export const selectMessages = createSelector(selectMessagesBase, (items) => items)

// Methods

// Main Exports
export const { addMessage, clearMessages, deleteMessage } = appSlice.actions;
export default appSlice.reducer;
