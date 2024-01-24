import * as React from "react"
import type { HeadFC, PageProps } from "gatsby"
import Layout from "../components/layout"
import { useEffect } from "react"
import { useAppDispatch } from "../context"
import { connectToHub } from "../features/hub/hubSlice"

const IndexPage: React.FC<PageProps> = () => {
  const dispatch = useAppDispatch()

  useEffect(() => {
    dispatch(connectToHub());
  }, [])

  return (
    <Layout>
      <div className="container">
        <div className="row">
          <div className="col">
            <h1>uvroberta.com</h1>
          </div>
        </div>
      </div>
    </Layout>
  )
}

export default IndexPage

export const Head: HeadFC = () => <title>Home | uvroberta.com</title>
