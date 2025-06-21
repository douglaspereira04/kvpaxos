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

    template<OperationType TYPE>
    static TrackingInfo<T>* get_tracking_info(Operation<T>* operation){
        TrackingInfo<T>* tracking_info;
        constexpr OperationType type = generic_type<TYPE>();
        tracking_info = new TrackingInfo<T>(type);
        if constexpr(
            type == GET || type == SET || 
            type == SCAN || type == DEL
        ) {
            tracking_info->__key = operation->key();
        }
    
        if constexpr(type == SCAN) {
            tracking_info->__len = static_cast<ScanOperation<T>*>(operation)->len();
        } 
        return tracking_info;
    }

    inline OperationType type(){
        return __type;
    }

    inline T &key(){
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

