#ifndef __LORAMAC_CONFIRMQUEUE_H__
#define __LORAMAC_CONFIRMQUEUE_H__


/*!
 * LoRaMac MLME-Confirm queue length
 */
#define LORA_MAC_MLME_CONFIRM_QUEUE_LEN             5

/*!
 * Structure to hold multiple MLME request confirm data
 */
typedef struct sMlmeConfirmQueue
{
    /*!
     * Holds the previously performed MLME-Request
     */
    Mlme_t Request;
    /*!
     * Status of the operation
     */
    LoRaMacEventInfoStatus_t Status;
    /*!
     * Set to true, if the request is ready to be handled
     */
    bool ReadyToHandle;
    /*!
     * Set to true, if it is not permitted to set the ReadyToHandle variable
     * with a function call to LoRaMacConfirmQueueSetStatusCmn.
     */
    bool RestrictCommonReadyToHandle;
}MlmeConfirmQueue_t;

/*!
 * \brief   Initializes the confirm queue
 *
 * \param   [IN] primitives - Pointer to the LoRaMac primitives.
 */
void LoRaMacConfirmQueueInit( LoRaMacPrimitives_t* primitives );

/*!
 * \brief   Adds an element to the confirm queue.
 *
 * \param   [IN] mlmeConfirm - Pointer to the element to add.
 *
 * \retval  [true - operation was successful, false - operation failed]
 */
bool LoRaMacConfirmQueueAdd( MlmeConfirmQueue_t* mlmeConfirm );

/*!
 * \brief   Removes the last element which was added into the queue.
 *
 * \retval  [true - operation was successful, false - operation failed]
 */
bool LoRaMacConfirmQueueRemoveLast( void );

/*!
 * \brief   Removes the first element which was added to the confirm queue.
 *
 * \retval  [true - operation was successful, false - operation failed]
 */
bool LoRaMacConfirmQueueRemoveFirst( void );

/*!
 * \brief   Sets the status of an element.
 *
 * \param   [IN] status - The status to set.
 *
 * \param   [IN] request - The related request to set the status.
 */
void LoRaMacConfirmQueueSetStatus( LoRaMacEventInfoStatus_t status, Mlme_t request );

/*!
 * \brief   Gets the status of an element.
 *
 * \param   [IN] request - The request to query the status.
 *
 * \retval  The status of the related MlmeRequest.
 */
LoRaMacEventInfoStatus_t LoRaMacConfirmQueueGetStatus( Mlme_t request );

/*!
 * \brief   Sets a common status for all elements in the queue.
 *
 * \param   [IN] status - The status to set.
 */
void LoRaMacConfirmQueueSetStatusCmn( LoRaMacEventInfoStatus_t status );

/*!
 * \brief   Gets the common status of all elements.
 *
 * \retval  The common status of all elements.
 */
LoRaMacEventInfoStatus_t LoRaMacConfirmQueueGetStatusCmn( void );

/*!
 * \brief   Verifies if a request is in the queue and active.
 *
 * \param   [IN] request - The request to verify.
 *
 * \retval  [true - element is in the queue, false - element is not in the queue].
 */
bool LoRaMacConfirmQueueIsCmdActive( Mlme_t request );

/*!
 * \brief   Handles all callbacks of active requests
 *
 * \param   [IN] mlmeConfirm - Pointer to the generic mlmeConfirm structure.
 */
void LoRaMacConfirmQueueHandleCb( MlmeConfirm_t* mlmeConfirm );

/*!
 * \brief   Query number of elements in the queue.
 *
 * \retval  Number of elements.
 */
uint8_t LoRaMacConfirmQueueGetCnt( void );

/*!
 * \brief   Verify if the confirm queue is full.
 *
 * \retval  [true - queue is full, false - queue is not full].
 */
bool LoRaMacConfirmQueueIsFull( void );

#endif // __LORAMAC_CONFIRMQUEUE_H__
