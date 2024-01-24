import * as React from "react"
import type { HeadFC, PageProps } from "gatsby"
import Layout from "../components/layout"
import AzMap from "../components/azMap"
import { useEffect, useState } from "react"
import * as Styles from "../styles"
import Webcam from "../components/webcam"
import { useAppDispatch } from "../context"
import { connectToHub } from "../features/hub/hubSlice"

const Dashboard: React.FC<PageProps> = () => {
  const [isMapFocused, setIsMapFocused] = useState(true);
  const dispatch = useAppDispatch()

  useEffect(() => {
    dispatch(connectToHub());
  }, [])

  return (
    <Layout padTop={false}>

      <div style={(isMapFocused) ? Styles.DashBack : Styles.DashFront}>
        <AzMap />
      </div>

      <div id="dashCam" style={(isMapFocused) ? Styles.DashFront : Styles.DashBack} title="Click to toggle view" onClick={() => { setIsMapFocused(!isMapFocused) }}>
        <Webcam src={process.env.GATSBY_WEBCAM_URL} />
      </div>

    </Layout>
  )
}

export default Dashboard

export const Head: HeadFC = () => <title>Dashboard | uvroberta.com</title>
