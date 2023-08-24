import React, { ComponentPropsWithoutRef } from "react";
import { useIsAuthenticated } from "@azure/msal-react";

interface INotAuthenticated extends ComponentPropsWithoutRef<'div'> { }

const NotAuthenticated = ({ children }: INotAuthenticated) => {
    const isAuthenticated = useIsAuthenticated();
    if (!isAuthenticated)
        return <>{children}</>
    return null
}

export default NotAuthenticated