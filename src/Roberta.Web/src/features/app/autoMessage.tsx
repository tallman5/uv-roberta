import React, { ComponentPropsWithoutRef, useEffect, useState } from "react";
import { deleteMessage, Message } from "./appSlice";
import { useAppDispatch } from "../../context";
import { Expander, TimeoutBar } from "@tallman/strong-strap";

interface IAutoMessage extends ComponentPropsWithoutRef<'div'> {
    message: Message
}

const AutoMessage = ({ message }: IAutoMessage) => {
    const dispatch = useAppDispatch()
    const [isExpanded, setIsExpanded] = useState(false)

    const collapse = () => {
        setIsExpanded(false)
        window.setTimeout(() => {
            dispatch(deleteMessage(message.id))
        }, 1000)
    }

    useEffect(() => {
        if (!message) return
        if (message.displayTimeout > 0)
            window.setTimeout(collapse, message.displayTimeout)
        setIsExpanded(true)
    }, [message])

    return (
        <Expander isExpanded={isExpanded}>
            <div className={'alert alert-' + message.uiFunction}>
                {
                    (message.title && message.title.length > 0)
                        ? <div style={{ display: 'flex', alignItems: 'center' }}>
                            <div style={{ minWidth: '150px', flexGrow: 1, marginRight: '10px' }}>{message.title}</div>
                            <button type="button" onClick={() => { collapse() }} className="btn-close" data-bs-dismiss="alert" aria-label="Close" />
                        </div>
                        : null
                }
                {
                    (message.details && message.details.length > 0)
                        ? <div>
                            <hr />
                            <div dangerouslySetInnerHTML={{ __html: message.details }}></div>
                        </div>
                        : null
                }
                {
                    (message.displayTimeout > 0)
                        ? <div>
                            <hr />
                            <TimeoutBar duration={message.displayTimeout} uiFunction={message.uiFunction} />
                        </div>
                        : null
                }
            </div>
        </Expander>
    )
}

export default AutoMessage