//
//  tipiki.cpp
//  tipiki
//
//  Created by Rodrigo Agustin Peinado on 5/7/16.
//  Copyright © 2016 Rodrigo Agustin Peinado. All rights reserved.
//
#include "tipiki.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <jvmti.h>
#include "stdlib.h"
#define countof(array) (sizeof(array)/sizeof(array[0]))

using namespace std;

bool setsecuritymanager_checked=false;

static void check(jvmtiEnv *jvmti, jvmtiError errnum, const char *str)
{
    if ( errnum != JVMTI_ERROR_NONE ) {
        char *errnum_str = NULL;
        jvmti->GetErrorName(errnum, &errnum_str);
        
        cout << "ERROR: JVMTI";
    }
}

static void printObject(jvmtiEnv *jvmti_env, jthread thread, jvmtiLocalVariableEntry* entry) {
    jvmtiError error;
    jint int_value_ptr;
    jlong long_value_ptr;
    jfloat float_value_ptr;
    jdouble double_value_ptr;
    jobject object_value_ptr;
    jint depht = 0;
    
    cout << entry->signature << ":" << entry->name << endl;
    
    switch (entry->signature[0]) {
        case 'Z' :
        case 'B' :
        case 'S' :
        case 'C' :
        case 'I' :
            error = jvmti_env->GetLocalInt(thread, depht, entry->slot, &int_value_ptr);
            if(error == JVMTI_ERROR_NONE) {
                cout << "value: " << int_value_ptr << endl;
            } else {
                cout << "error: " << error << endl;
            }
            break;
        case 'J' :
            error = jvmti_env->GetLocalLong(thread, depht, entry->slot, &long_value_ptr);
            if(error == JVMTI_ERROR_NONE) {
                cout << "value: " << long_value_ptr << endl;
            } else {
                cout << "error: " << error << endl;
            }
            break;
        case 'F' :
            error = jvmti_env->GetLocalFloat(thread, depht, entry->slot, &float_value_ptr);
            if(error == JVMTI_ERROR_NONE) {
                cout << "value: " << float_value_ptr << endl;
            } else {
                cout << "error: " << error << endl;
            }
            break;
        case 'D' :
            error = jvmti_env->GetLocalDouble(thread, depht, entry->slot, &double_value_ptr);
            if(error == JVMTI_ERROR_NONE) {
                cout << "value: " << double_value_ptr << endl;
            } else {
                cout << "error: " << error << endl;
            }
            break;
        case 'L' :
            error = jvmti_env->GetLocalObject(thread, depht, entry->slot, &object_value_ptr);
            if(error == JVMTI_ERROR_NONE) {
                // get object attributes
                
                cout << "value: " << object_value_ptr << endl;
            } else {
                cout << "error: " << error << endl;
            }
            break;
    }
}

static void JNICALL Exception(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread,jmethodID method,jlocation location,jobject exception,jmethodID catch_method,jlocation catch_location)
{
    char* method_name;
    char* method_signature;
    char* generic_ptr_method;
    jclass klass;
    jint type;
    jint entryCount;
    jvmtiError error;
    jvmtiLocalVariableEntry* localVariableEntry;
    localVariableEntry = NULL;
    
    type=jni_env->GetObjectRefType(exception);
    
    cout << "type=" << type << endl;
    
    /*
    jvmtiFrameInfo frames[2];
    jint count;
    error = (*jvmti_env).GetStackTrace(NULL, 0, 1, (jvmtiFrameInfo*)&frames, &count);
    cout << "count=" << count << endl;
    */
    
    if(type>0)
    {
        klass=jni_env->GetObjectClass(exception);
        jvmti_env->GetMethodName(method,&method_name,&method_signature,&generic_ptr_method);
        
        cout << "Method name= " << method_name << endl;
        
        //method_ptr = (Method*)method;
        //count = method_ptr->get_local_var_table_size();
        
        error = jvmti_env->GetLocalVariableTable(method, &entryCount, &localVariableEntry);
        //error = jvmti_env->GetLocalVariableTable(frames[0].method, &entryCount, &localVariableEntry);
        
        if(error == JVMTI_ERROR_NONE) {
            
            jvmtiLocalVariableEntry* entry = localVariableEntry;
            cout << "entryCount=" << entryCount << endl;
            
            for(int i = 0; i < entryCount; i++, entry++) {
                printObject(jvmti_env, thread, entry);
                
                jvmti_env->Deallocate(reinterpret_cast<unsigned char*>(entry->signature));
                jvmti_env->Deallocate(reinterpret_cast<unsigned char*>(entry->name));
                jvmti_env->Deallocate(reinterpret_cast<unsigned char*>(entry->generic_signature));
            }
            
            jvmti_env->Deallocate(reinterpret_cast<unsigned char*>(localVariableEntry));
            
        } else if(error == JVMTI_ERROR_ABSENT_INFORMATION) {
            cout << "<NO LOCAL VARIABLE INFORMATION AVAILABLE>" << endl;
        } else {
            cout << "<ERROR>" << endl;
        }
        
    }
}

/*
static void JNICALL loadClass(jvmtiEnv *jvmti_env,JNIEnv* jni_env,jthread thread,jclass klass)
{
    char *class_name;
    char *generic_ptr_class;
    
    jvmti_env->GetClassSignature(klass, &class_name,&generic_ptr_class);
    
    if(strcmp("LHello;",class_name)==0)
    {
        cout << "Agent_Registered" << endl;
        jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION,(jthread)NULL);
        jvmti_env->SetEventNotificationMode(JVMTI_DISABLE,JVMTI_EVENT_CLASS_PREPARE,(jthread)NULL);
    }
    
    jvmti_env->Deallocate((unsigned char *)class_name);
    jvmti_env->Deallocate((unsigned char *)generic_ptr_class);
}
*/


JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm, char *options, void *reserved)
{
    static jvmtiEnv *jvmti=NULL;
    static jvmtiCapabilities capabilities;
    jvmtiEventCallbacks callbacks;
    jint res;
    
    
    cout << "Agent_OnLoad" << endl;
    
    res = vm->GetEnv((void **)&jvmti, JVMTI_VERSION_1);
    if (res != JNI_OK||jvmti==NULL)
    {
        cout << "ERROR: Unable to access JVMTI Version 1" << endl;
    }
    
    
    (void)memset(&capabilities,0, sizeof(capabilities));
    
    capabilities.can_generate_exception_events=1;
    capabilities.can_get_line_numbers=1;
    capabilities.can_access_local_variables=1;
    capabilities.can_tag_objects=1;
    
    jvmtiError error = jvmti->AddCapabilities(&capabilities);
    
    check(jvmti,error,"Unable to get necessary capabilities.");
    
    (void)memset(&callbacks,0, sizeof(callbacks));
    
    //callbacks.ClassPrepare = &loadClass;
    
    callbacks.Exception=&Exception;
    
    jvmti->SetEventCallbacks(&callbacks, (jint)sizeof(callbacks));
    
    //jvmti->SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_CLASS_PREPARE,(jthread)NULL);
    jvmti->SetEventNotificationMode(JVMTI_ENABLE,JVMTI_EVENT_EXCEPTION,(jthread)NULL);
    
    return JNI_OK;
}


JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm)
{
    cout << "Agent_OnUnload" << endl;
    
}
