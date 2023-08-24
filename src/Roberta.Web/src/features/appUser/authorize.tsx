import React, { ComponentPropsWithoutRef, useEffect, useState } from "react";
import { useAppSelector } from "../../context";
import { selectAppUser } from "./appUserSlice";

interface IAuthorize extends ComponentPropsWithoutRef<'div'> {
    requireAdmin?: boolean
    requireDevelopment?: boolean
    requireProduction?: boolean
    requireRole?: string
}

const Authorize = ({
    children,
    requireAdmin = false,
    requireDevelopment = false,
    requireProduction = false,
    requireRole = ''
}: IAuthorize) => {
    const [willRender, setWillRender] = useState(false);
    const appUser = useAppSelector(selectAppUser);

    useEffect(() => {
        let wr = true;

        if (!appUser) {
            wr = false;
        }

        if (requireDevelopment === true) {
            if (process.env.NODE_ENV !== "development")
                wr = false;
        }

        if (requireProduction === true) {
            if (process.env.NODE_ENV !== "production")
                wr = false;
        }

        if (requireAdmin === true) {
            if (appUser && appUser.roles && appUser.roles.length > 0
                && appUser.roles.includes("roberta.admins")) {
            }
            else {
                wr = false;
            }
        }

        if (requireRole.length > 0) {
            if (appUser && appUser.roles && appUser.roles.length > 0
                && appUser.roles.includes(requireRole)) {
            }
            else {
                wr = false;
            }
        }

        setWillRender(wr);
    }, [appUser])

    if (willRender === true)
        return <>{children}</>
    else
        return null
}

export default Authorize
