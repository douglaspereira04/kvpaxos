#ifndef WORKLOAD_TRACKING_INFO_H
#define WORKLOAD_TRACKING_INFO_H

#include "operation.hpp"
#include "get_operation.hpp"
#include "set_operation.hpp"
#include "scan_operation.hpp"
#include "del_operation.hpp"
#include "get_callback_operation.hpp"
#include "set_callback_operation.hpp"
#include "scan_callback_operation.hpp"
#include "del_callback_operation.hpp"


namespace workload {


template<typename T>
class TrackingInfo {
public:
    TrackingInfo(OperationType type){
        __type = type;
    }

    template<OperationType type>
    static TrackingInfo<T>* get_tracking_info(Operation<T>* operation){
        TrackingInfo<T>* tracking_info;
        if constexpr(
            type == GET || type == GET_CALLBACK ||
            type == SET || type == SET_CALLBACK ||
            type == SCAN || type == SCAN_CALLBACK ||
            type == DEL || type == DEL_CALLBACK
        ) {
            constexpr OperationType generic_type = static_cast<OperationType>(static_cast<int>(type) & ~0x1);
            tracking_info = new TrackingInfo<T>(generic_type);
            tracking_info->__key = operation->key();
        } else {
            tracking_info = new TrackingInfo<T>(type);
        }

    
        if constexpr(type == SCAN || type == SCAN_CALLBACK) {
            tracking_info->__len = static_cast<ScanOperation<T>*>(operation)->len();
        } 
        return tracking_info;
    }

    inline OperationType type(){
        return __type;
    }

    inline T key(){
        return __key;
    }

    inline size_t len(){
        return __len;
    }

private:
    OperationType __type;
    T __key;
    size_t __len;
};

}

#endif

