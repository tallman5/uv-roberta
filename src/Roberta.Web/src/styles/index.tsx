import { CSSProperties } from 'react'

export const DashFrontScale = 20;

export const Cover: CSSProperties = {
    height: '100%',
    left: 0,
    objectFit: 'cover',
    position: 'absolute',
    top: 0,
    width: '100%',
}

export const DashBack: CSSProperties = {
    height: '100vh',
    width: '100vw',
    zIndex: 0,
}

export const DashFront: CSSProperties = {
    bottom: 0,
    height: (DashFrontScale * 9) + 'px',
    left: 0,
    position: 'absolute',
    width: (DashFrontScale * 16) + 'px',
    zIndex: 10,
}
