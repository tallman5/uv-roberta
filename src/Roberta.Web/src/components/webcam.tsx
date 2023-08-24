import React, { ComponentPropsWithoutRef, useEffect, useState } from 'react'
import * as Styles from '../styles'

export interface ILayout extends ComponentPropsWithoutRef<'div'> {
    src?: string,
}

const Webcam = ({ src }: ILayout) => {
    return (
        <img style={Styles.Cover} src={src} alt='Roberta Webcam' />
    )
}

export default Webcam
