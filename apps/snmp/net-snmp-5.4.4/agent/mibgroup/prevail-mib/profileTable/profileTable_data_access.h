/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 14170 $ of $
 *
 * $Id:$
 */
#ifndef PROFILETABLE_DATA_ACCESS_H
#define PROFILETABLE_DATA_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif


/* *********************************************************************
 * function declarations
 */

/* *********************************************************************
 * Table declarations
 */
/**********************************************************************
 **********************************************************************
 ***
 *** Table profileTable
 ***
 **********************************************************************
 **********************************************************************/
/*
 * prevail-mib::profileTable is subid 8 of modEoCMib.
 * Its status is Current.
 * OID: .1.3.6.1.4.1.36186.8.8, length: 9
*/


    int profileTable_init_data(profileTable_registration * profileTable_reg);


    /*
     * TODO:180:o: Review profileTable cache timeout.
     * The number of seconds before the cache times out
     */
#define PROFILETABLE_CACHE_TIMEOUT   60

void profileTable_container_init(netsnmp_container **container_ptr_ptr,
                             netsnmp_cache *cache);
void profileTable_container_shutdown(netsnmp_container *container_ptr);

int profileTable_container_load(netsnmp_container *container);
void profileTable_container_free(netsnmp_container *container);

int profileTable_cache_load(netsnmp_container *container);
void profileTable_cache_free(netsnmp_container *container);

    /*
    ***************************************************
    ***             START EXAMPLE CODE              ***
    ***---------------------------------------------***/
/* *********************************************************************
 * Since we have no idea how you really access your data, we'll go with
 * a worst case example: a flat text file.
 */
#define MAX_LINE_SIZE 256
    /*
    ***---------------------------------------------***
    ***              END  EXAMPLE CODE              ***
    ***************************************************/
    int profileTable_row_prep( profileTable_rowreq_ctx *rowreq_ctx);



#ifdef __cplusplus
}
#endif

#endif /* PROFILETABLE_DATA_ACCESS_H */
