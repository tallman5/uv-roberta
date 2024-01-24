import { isBrowser } from '@tallman/strong-strap';
import { Link } from 'gatsby'
import { StaticImage } from 'gatsby-plugin-image'
import React, { useEffect, useState } from 'react'
import AccountDropdown from '../features/appUser/accountDropdown';
import Authorize from '../features/appUser/authorize';

const Header = () => {
    const [navMenuVisible, setNavMenuVisible] = useState(false)
    const [pathname, setPathname] = useState('')

    useEffect(() => {
        if (isBrowser) {
            setPathname(window.location.pathname.toLowerCase())
        }
    }, [])

    return (
        <header className='bg-light'>
            <nav className="navbar navbar-expand-md fixed-top" style={{ backgroundColor: "#ffffffaa" }}>
                <div className="container">
                    <Link className="navbar-brand" to='/'>
                        <StaticImage src='../images/icon.png' alt='Roberta' width={32} />
                        <span style={{ marginLeft: '5px', marginTop: '-5px' }}>uvroberta.com</span>
                    </Link>
                    <button className={"navbar-toggler"} type="button" data-bs-toggle="collapse"
                        data-bs-target="#navbarSupportedContent" aria-controls="navbarSupportedContent"
                        aria-expanded="false" aria-label="Toggle navigation"
                        onClick={() => { setNavMenuVisible(!navMenuVisible) }}>
                        <span className="navbar-toggler-icon"></span>
                    </button>
                    <div style={{ justifyContent: 'space-between' }} className={(navMenuVisible === true) ? 'collapse navbar-collapse show' : 'collapse navbar-collapse'} id="navbarNavAltMarkup">
                        <div className={'navbar-nav'}>
                            <Link className={(pathname.startsWith('/dashboard')) ? 'nav-link active' : 'nav-link'} to="/dashboard">DASHBOARD</Link>
                            <Authorize requireDevelopment={true} requireRole='roberta.admins'>
                                <Link className={(pathname.startsWith('/rob-state')) ? 'nav-link active' : 'nav-link'} to="/rob-state">STATE</Link>
                                <Link className={(pathname.startsWith('/scratch')) ? 'nav-link active' : 'nav-link'} to="/scratch">SCRATCH</Link>
                            </Authorize>
                            {
                                (process.env.NODE_ENV === "development")
                                    ? <>
                                    </>
                                    : null
                            }
                        </div>
                        <div className='d-flex'>
                            <AccountDropdown />
                        </div>
                    </div>
                </div>
            </nav>
        </header>
    )
}

export default Header