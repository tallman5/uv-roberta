import React from "react";
import type { HeadFC, PageProps } from "gatsby"
import Layout from "../../components/layout"
import { useMsal } from "@azure/msal-react";
import { Stringify } from "@tallman/strong-strap";
import { useAppSelector } from "../../context";
import { selectAccessToken, selectAppUser } from "../../features/appUser/appUserSlice";

const ScratchIndex: React.FC<PageProps> = () => {
    const { accounts } = useMsal();
    const accessToken = useAppSelector(selectAccessToken);
    const appUser = useAppSelector(selectAppUser);

    return (
        <Layout padTop={true}>
            <div className="container">
                <div className="row">
                    <div className="col">
                        <h1>Scratch</h1>
                    </div>
                </div>
                <div className="row">
                    <div className="col">
                        <h2>Account</h2>
                        <div>
                            {
                                (accounts && accounts.length > 0)
                                    ? <Stringify o={accounts[0]} />
                                    : 'No Accounts'
                            }
                        </div>
                    </div>
                </div>
                <div className="row">
                    <div className="col">
                        <h2>Access Token</h2>
                        <div>
                            {
                                (accessToken)
                                    ? <Stringify o={accessToken} />
                                    : 'No Token'
                            }
                        </div>
                    </div>
                </div>
                <div className="row">
                    <div className="col">
                        <h2>App User</h2>
                        <div>
                            <Stringify o={appUser} />
                        </div>
                    </div>
                </div>
            </div>
        </Layout>
    )
}

export default ScratchIndex

export const Head: HeadFC = () => <title>Scratch | uvroberta.com</title>
