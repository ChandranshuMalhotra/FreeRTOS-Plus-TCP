/*
 * RM46L852.c
 *
 *  Created on: Jul 10, 2023
 *      Author: chmalho
 */

#include "FreeRTOS.h"
#include "list.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

#include "emac.h"
#include "mdio.h"
#include "phy_dp83640.h"
#include "sci.h"

#define DRIVER_READY    ( 0x40 )
#if ipconfigETHERNET_DRIVER_FILTERS_FRAME_TYPES == 0
    #define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer )    eConsiderFrameForProcessing( ( pucEthernetBuffer ) )
#else
    #define ipCONSIDER_FRAME_FOR_PROCESSING( pucEthernetBuffer )    eProcessBuffer
#endif

struct hdkif_struct xHdkif;
TaskHandle_t receiveTaskHandle = NULL;

void prvEMACHandlerTask( void * pvParameters  );
void prvProcessFrame( uint32 length, uint32 bufptr );
//struct hdkif_t xHdkif;
//struct pbuf_t xpbuf;
uint8   emacAddress[6U] =   {0x00U, 0x08U, 0xEEU, 0x03U, 0xA6U, 0x6CU};

BaseType_t xNetworkInterfaceInitialise( void )
{
//    xHdkif.inst_num = 0;
//    hdkif_inst_config();
    BaseType_t xFirstCall = pdTRUE;
    BaseType_t xTaskCreated;

    if (EMACHWInit(emacAddress) == EMAC_ERR_OK)
    {
        //sci_print("\n network initialise successful\n");
        if( ( xFirstCall == pdTRUE ) || ( receiveTaskHandle == NULL ) )
        {
            xTaskCreated = xTaskCreate( prvEMACHandlerTask,
                                        "EMAC-Handler",
                                        configMINIMAL_STACK_SIZE * 2,
                                        NULL,
                                        configMAX_PRIORITIES - 1,
                                        &receiveTaskHandle );

            if( ( receiveTaskHandle == NULL ) || ( xTaskCreated != pdPASS ) )
            {
                FreeRTOS_printf( ( "Failed to create the handler task." ) );

            }
            /* After this, the task should not be created. */
            else
                xFirstCall = pdFALSE;
        }

//        xTaskCreate( prvEMACHandlerTask,
//                        "EMAC-Handler",
//                        configMINIMAL_STACK_SIZE * 3,
//                        NULL,
//                        configMAX_PRIORITIES - 1,
//                        NULL);
//
        return pdPASS;
    }
    else
    {
        sci_print("\n network initialise unsuccessful\n");
        return pdFAIL;
    }
}
BaseType_t xNetworkInterfaceOutput( NetworkBufferDescriptor_t * const pxNetworkBuffer,
                                    BaseType_t xReleaseAfterSend )
{
       struct pbuf_struct xpbuf;
      xpbuf.payload = pxNetworkBuffer->pucEthernetBuffer;
      xpbuf.len = pxNetworkBuffer->xDataLength;
      xpbuf.tot_len = pxNetworkBuffer->xDataLength;
      xpbuf.next = NULL;
      if(xpbuf.tot_len < MIN_PKT_LEN) {
          xpbuf.tot_len = MIN_PKT_LEN;
          xpbuf.len = MIN_PKT_LEN;
        }

      xHdkif.emac_base = EMAC_0_BASE;
      xHdkif.emac_ctrl_base = EMAC_CTRL_0_BASE;
      xHdkif.emac_ctrl_ram = EMAC_CTRL_RAM_0_BASE;
      xHdkif.mdio_base = MDIO_0_BASE;
      xHdkif.phy_addr = 1;
      xHdkif.phy_autoneg = Dp83640AutoNegotiate;
      xHdkif.phy_partnerability = Dp83640PartnerAbilityGet;
      xHdkif.mac_addr[0] = 0x00U;
      xHdkif.mac_addr[1] = 0x08U;
      xHdkif.mac_addr[2]= 0xEEU;
      xHdkif.mac_addr[3]= 0x03U;
      xHdkif.mac_addr[4]= 0xA6U;
      xHdkif.mac_addr[5]= 0x6CU;
      taskENTER_CRITICAL();
      {
          EMACTransmit(&xHdkif,&xpbuf);
      }
      taskEXIT_CRITICAL();

//      if (EMACTransmit(&xHdkif,&xpbuf) == TRUE)
////          sci_print("\nnetwork output success\n");
//      else
//          sci_print("\nnetwork output fail\n");

      if( xReleaseAfterSend == pdTRUE )
          {
              vReleaseNetworkBufferAndDescriptor( pxNetworkBuffer );
          }
      return pdPASS;
}

void prvEMACHandlerTask( void * pvParameters  )
{


    /* Wait for the driver to finish starting. */
    ( void ) ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

    while( pdTRUE )
    {
        ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS( 500 ) );
//        {
  //          sci_print("\ninside emac handler task\r\n");
//        }
//        else
//        {
            //BaseType_t receiving = pdTRUE;

            /* A packet was received, the link must be up. */
//            if( length )
//            {
//                prvProcessFrame( length,bufptr );
//            }
//            else
//                sci_print("\nPacket with zero length\n");
        taskENTER_CRITICAL();
              {
            EMACReceive(&hdkif_data[0U]);
              }
              taskEXIT_CRITICAL();

        //}
    }
}


void prvProcessFrame( uint32 length, uint32 bufptr)
{
    //sci_print("\ninside process frame\r\n");
    NetworkBufferDescriptor_t * pxBufferDescriptor = pxGetNetworkBufferWithDescriptor( length, 0 );

    if( pxBufferDescriptor != NULL )
    {
       // ENET_ReadFrame( ethernetifLocal->base, &( ethernetifLocal->handle ), pxBufferDescriptor->pucEthernetBuffer, length, 0, NULL );
        pxBufferDescriptor->xDataLength = length;
//        taskENTER_CRITICAL();
//        {
            ( void ) memcpy( pxBufferDescriptor->pucEthernetBuffer,
                         bufptr,
                         length );
//        }
//        taskEXIT_CRITICAL();

        if( ipCONSIDER_FRAME_FOR_PROCESSING( pxBufferDescriptor->pucEthernetBuffer ) == eProcessBuffer )
        {
            IPStackEvent_t xRxEvent;
            xRxEvent.eEventType = eNetworkRxEvent;
            xRxEvent.pvData = ( void * ) pxBufferDescriptor;
            sci_print("\r\nsending event struct to IP\n");
            if( xSendEventStructToIPTask( &xRxEvent, 0 ) == pdFALSE )
            {
                vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
                iptraceETHERNET_RX_EVENT_LOST();
                FreeRTOS_printf( ( "RX Event Lost\n" ) );
            }
        }
        else
        {
            #if ( ( ipconfigHAS_DEBUG_PRINTF == 1 ) && defined( FreeRTOS_debug_printf ) )
                const EthernetHeader_t * pxEthernetHeader;
                char ucSource[ 18 ];
                char ucDestination[ 18 ];

                pxEthernetHeader = ( ( const EthernetHeader_t * ) pxBufferDescriptor->pucEthernetBuffer );


                FreeRTOS_EUI48_ntop( pxEthernetHeader->xSourceAddress.ucBytes, ucSource, 'A', ':' );
                FreeRTOS_EUI48_ntop( pxEthernetHeader->xDestinationAddress.ucBytes, ucDestination, 'A', ':' );

                FreeRTOS_debug_printf( ( "Invalid target MAC: dropping frame from: %s to: %s", ucSource, ucDestination ) );
            #endif /* if ( ( ipconfigHAS_DEBUG_PRINTF == 1 ) && defined( FreeRTOS_debug_printf ) ) */
            vReleaseNetworkBufferAndDescriptor( pxBufferDescriptor );
            /* Not sure if a trace is required.  The stack did not want this message */
        }
    }
    else
    {
        #if ( ( ipconfigHAS_DEBUG_PRINTF == 1 ) && defined( FreeRTOS_debug_printf ) )
            FreeRTOS_debug_printf( ( "No Buffer Available: dropping incoming frame!!" ) );
        #endif
        //ENET_ReadFrame( ENET, &( ethernetifLocal->handle ), NULL, length, 0, NULL );

        /* No buffer available to receive this message */
        iptraceFAILED_TO_OBTAIN_NETWORK_BUFFER();
    }
}

void EMACReceive(hdkif_t *hdkif)
{

    rxch_t *rxch_int;
  volatile emac_rx_bd_t *curr_bd, *curr_tail, *last_bd;


  /* The receive structure that holds data about a particular receive channel */
  rxch_int = &(hdkif->rxchptr);

  /* Get the buffer descriptors which contain the earliest filled data */
  /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
  curr_bd = rxch_int->active_head;
  /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
  last_bd = rxch_int->active_tail;

  /**
   * Process the descriptors as long as data is available
   * when the DMA is receiving data, SOP flag will be set
  */
  /*SAFETYMCUSW 28 D MR:NA <APPROVED> "Hardware status bit read check" */
  /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
  /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
  while((curr_bd->flags_pktlen & EMAC_BUF_DESC_SOP) == EMAC_BUF_DESC_SOP) {

     //prvProcessFrame(curr_bd->flags_pktlen,curr_bd->bufptr);
     //prvProcessFrame(curr_bd->bufoff_len, curr_bd->bufptr);

      //prvProcessFrame(curr_bd->bufoff_len,curr_bd->bufptr);

    /* Start processing once the packet is loaded */
    /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
    /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
    if((curr_bd->flags_pktlen & EMAC_BUF_DESC_OWNER)
       != EMAC_BUF_DESC_OWNER) {

      /* this bd chain will be freed after processing */
      /*SAFETYMCUSW 71 S MR:17.6 <APPROVED> "Assigned pointer value has required scope." */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      rxch_int->free_head = curr_bd;
      prvProcessFrame(curr_bd->bufoff_len & 0x0000FFFF, curr_bd->bufptr);
      /* Get the total length of the packet. curr_bd points to the start
       * of the packet.
       */

      /*
       * The loop runs till it reaches the end of the packet.
       */
      /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      while((curr_bd->flags_pktlen & EMAC_BUF_DESC_EOP)!= EMAC_BUF_DESC_EOP)
      {

          /*Update the flags for the descriptor again and the length of the buffer*/
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        curr_bd->flags_pktlen = (uint32)EMAC_BUF_DESC_OWNER;
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        curr_bd->bufoff_len = (uint32)MAX_TRANSFER_UNIT;
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        last_bd = curr_bd;
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        curr_bd = curr_bd->next;
      }

      /* Updating the last descriptor (which contained the EOP flag) */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */

      curr_bd->flags_pktlen = (uint32)EMAC_BUF_DESC_OWNER;

      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        curr_bd->bufoff_len = (uint32)MAX_TRANSFER_UNIT;
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        last_bd = curr_bd;
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        curr_bd = curr_bd->next;

      /* Acknowledge that this packet is processed */
      /*SAFETYMCUSW 439 S MR:11.3 <APPROVED> "Address stored in pointer is passed as as an int parameter. - Advisory as per MISRA" */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      EMACRxCPWrite(hdkif->emac_base, (uint32)EMAC_CHANNELNUMBER, (uint32)last_bd);

      /* The next buffer descriptor is the new head of the linked list. */
      /*SAFETYMCUSW 71 S MR:17.6 <APPROVED> "Assigned pointer value has required scope." */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      rxch_int->active_head = curr_bd;

      /* The processed descriptor is now the tail of the linked list. */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      curr_tail = rxch_int->active_tail;
    /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      curr_tail->next = rxch_int->free_head;

      /* The last element in the already processed Rx descriptor chain is now the end of list. */
      /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
      last_bd->next = NULL;


        /**
         * Check if the reception has ended. If the EOQ flag is set, the NULL
         * Pointer is taken by the DMA engine. So we need to write the RX HDP
         * with the next descriptor.
         */
        /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
        /*SAFETYMCUSW 134 S MR:12.2 <APPROVED> "LDRA Tool issue" */
      /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        if((curr_tail->flags_pktlen & EMAC_BUF_DESC_EOQ) == EMAC_BUF_DESC_EOQ) {
          /*SAFETYMCUSW 439 S MR:11.3 <APPROVED> "Address stored in pointer is passed as as an int parameter. - Advisory as per MISRA" */
          /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
          EMACRxHdrDescPtrWrite(hdkif->emac_base, (uint32)(rxch_int->free_head), (uint32)EMAC_CHANNELNUMBER);
        }

        /*SAFETYMCUSW 71 S MR:17.6 <APPROVED> "Assigned pointer value has required scope." */
        /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        /*SAFETYMCUSW 45 D MR:21.1 <APPROVED> "Valid non NULL input parameters are assigned in this driver" */
        rxch_int->free_head  = curr_bd;
        rxch_int->active_tail = last_bd;
      }
    }
}

void taskHandleYield()
{
    BaseType_t needsToYield = FALSE;
    configASSERT(receiveTaskHandle != NULL);
    vTaskNotifyGiveFromISR( receiveTaskHandle, &needsToYield );
    portYIELD_FROM_ISR( needsToYield );
}

//#define BUFFER_SIZE_ALLOC1               ( ipTOTAL_ETHERNET_FRAME_SIZE + ipBUFFER_PADDING )
//#define BUFFER_SIZE_ALLOC1_ROUNDED_UP    ( ( BUFFER_SIZE_ALLOC1 + 7 ) & ~0x07UL )
//static uint8_t ucBuffers[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ][ BUFFER_SIZE_ALLOC1_ROUNDED_UP ];
//
//void vNetworkInterfaceAllocateRAMToBuffers( NetworkBufferDescriptor_t pxNetworkBuffers[ ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS ] )
//{
//    BaseType_t x;
//
//    for( x = 0; x < ipconfigNUM_NETWORK_BUFFER_DESCRIPTORS; x++ )
//    {
//        /* pucEthernetBuffer is set to point ipBUFFER_PADDING bytes in from the
//         * beginning of the allocated buffer. */
//        pxNetworkBuffers[ x ].pucEthernetBuffer = &( ucBuffers[ x ][ ipBUFFER_PADDING ] );
//
//        /* The following line is also required, but will not be required in
//         * future versions. */
//        *( ( uint32_t * ) &ucBuffers[ x ][ 0 ] ) = ( uint32_t ) &( pxNetworkBuffers[ x ] );
//    }
//}


