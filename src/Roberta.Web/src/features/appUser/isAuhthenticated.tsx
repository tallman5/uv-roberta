import React, { ComponentPropsWithoutRef } from "react";
import { useIsAuthenticated } from "@azure/msal-react";

interface IIsAuthenticated extends ComponentPropsWithoutRef<'div'> { }

const IsAuthenticated = ({ children }: IIsAuthenticated) => {
    const isAuthenticated = useIsAuthenticated();
    if (isAuthenticated)
        return <>{children}</>
    return null
}

export default IsAuthenticated