import React, { useEffect, useState } from 'react'
import { Icon, KnownIcon, isBrowser, useMedia } from '@tallman/strong-strap';
import IsAuthenticated from './isAuhthenticated';
import NotAuthenticated from './notAuthenticated';
import { useMsal } from "@azure/msal-react";
import { loginRequest, scopes } from '../../context/authConfig';
import { clearToken, setAccessToken } from './appUserSlice';
import { useAppDispatch } from '../../context';
import { reconnectToHub } from '../hub/hubSlice';

const AccountDropdown = () => {
    const { accounts, instance } = useMsal();
    const alignMenu = useMedia(['(min-width: 767px)'], [' dropdown-menu-end'], '')
    const [dropdownVisible, setDropdownVisible] = useState(false)
    const dispatch = useAppDispatch();

    useEffect(() => {
        const windowClicked = () => {
            if (dropdownVisible)
                setDropdownVisible(false)
        }
        if (isBrowser) {
            window.addEventListener("click", windowClicked);
            return () => { window.removeEventListener("click", windowClicked); };
        }
    }, [dropdownVisible])

    const handleLogin = () => {
        instance.loginPopup(loginRequest)
            .then((response) => {
                if (response.account) {
                    const accessTokenRequest = {
                        scopes,
                        account: response.account,
                    };

                    instance.acquireTokenSilent(accessTokenRequest)
                        .then((accessTokenResponse) => {
                            dispatch(setAccessToken(accessTokenResponse.accessToken));
                        });
                }
            })
            .catch((e) => {
                console.log(e);
            });
    }

    const handleLogout = () => {
        dispatch(clearToken());
        instance.logoutPopup({
            postLogoutRedirectUri: "/",
        });
        dispatch(reconnectToHub());
    }

    return (
        <div className="dropdown">
            <button type="button" className={(dropdownVisible) ? 'btn dropdown-toggle show' : 'btn dropdown-toggle'}
                onClick={(e) => { e.stopPropagation(); setDropdownVisible(!dropdownVisible); }}>
                <Icon knownIcon={KnownIcon.PersonCircle} width={'16px'} fill={'currentcolor'} />&nbsp;&nbsp;
                {
                    (accounts && accounts.length > 0)
                        ? accounts[0].username
                        : 'guest'
                }
            </button>
            <div data-bs-popper={(dropdownVisible) ? "static" : ''} className={(dropdownVisible) ? 'dropdown-menu show' + alignMenu : 'dropdown-menu' + alignMenu}>
                <IsAuthenticated>
                    <button type="button" className="dropdown-item" onClick={() => handleLogout()}>Sign Out</button>
                </IsAuthenticated>
                <NotAuthenticated>
                    <button type="button" className="dropdown-item" onClick={() => handleLogin()}>Sign In</button>
                </NotAuthenticated>
            </div>
        </div>
    )
}

export default AccountDropdown