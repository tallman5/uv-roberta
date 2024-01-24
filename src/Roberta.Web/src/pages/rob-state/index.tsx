import * as React from "react"
import type { HeadFC, PageProps } from "gatsby"
import Layout from "../../components/layout"
import { connectToHub, selectConnectionStatus } from "../../features/hub/hubSlice"
import { useAppDispatch, useAppSelector } from "../../context"
import { Stringify, isBrowser } from "@tallman/strong-strap"
import { selectRoberta } from "../../features/roberta/robertaSlice"
import { useEffect } from "react"

const RobertaState: React.FC<PageProps> = () => {
    const dispatch = useAppDispatch()
    const connectionStatus = useAppSelector(selectConnectionStatus);
    const robertaState = useAppSelector(selectRoberta);

    useEffect(() => {
        if (!isBrowser) return;
        dispatch(connectToHub());
    }, [])

    return (
        <Layout>
            <div className="container">
                <div className="row align-items-center">
                    <div className="col">
                        <h1>Roberta State</h1>
                    </div>
                    <div className="col text-end">
                        Hub Status: {connectionStatus}
                    </div>
                </div>
                <div className="row">
                    <div className="col">
                        <h2>GPS State</h2>
                        <Stringify o={robertaState.gpsState} />
                    </div>
                </div>
            </div>
        </Layout>
    )
}

export default RobertaState

export const Head: HeadFC = () => <title>Roberta State | uvroberta.com</title>
