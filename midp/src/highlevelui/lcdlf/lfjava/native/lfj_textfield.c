/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

/**
 * @file
 *
 * Java native functions for TextField.
 */

#include <kni.h>
#include <sni.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>
#include <lfj_textfield.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpUtilKni.h>

#include <midpServices.h>
#include <stdio.h>

/**
 * Native text field context info
 */
typedef struct {
    MidpString text;
    int textLength;
    int caretPos;
} midp_textfield_info;

/**
* KNI function to launch native editor
* Class: javax.microedition.lcdui.TextFieldLFImpl
* Java prototype:
* private native int int keyCode,
        DynamicCharacterArray buffer,
        String textBoxTitle,
        int constraints,
        String initialInputMode,
        TextCursor cursor)
*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_TextFieldLFImpl_launchNativeTextField0() {
    MidpString text = NULL_MIDP_STRING;
    MidpString title = NULL_MIDP_STRING;
    MidpString inputMode = NULL_MIDP_STRING;
    int maxLength=0, caretPos=0, currentLength = 0, constraint=0, keyCode=0; 
    int result;
    MidpReentryData* pInfo;
    midp_textfield_info* pTextFieldInfo;
    jboolean needBlocking = KNI_FALSE;

    KNI_StartHandles(7);
    KNI_DeclareHandle(dynamicCharacterArrayObj);
    KNI_DeclareHandle(dynamicCharacterArrayClass);
    KNI_DeclareHandle(bufferJCharArray);
    KNI_DeclareHandle(inputModeJString);
    KNI_DeclareHandle(titleJString);
    KNI_DeclareHandle(TextCursorObj);
    KNI_DeclareHandle(TextCursorClass);

    keyCode = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, dynamicCharacterArrayObj);
    KNI_GetObjectClass(dynamicCharacterArrayObj, dynamicCharacterArrayClass);
    KNI_GetObjectField(dynamicCharacterArrayObj, 
                       KNI_GetFieldID(dynamicCharacterArrayClass, "buffer", "[C"), bufferJCharArray);
    KNI_GetParameterAsObject(3, titleJString);
    constraint = KNI_GetParameterAsInt(4);
    KNI_GetParameterAsObject(5, inputModeJString);
    KNI_GetParameterAsObject(6, TextCursorObj);
    KNI_GetObjectClass(TextCursorObj, TextCursorClass);

    pInfo = (MidpReentryData*)SNI_GetReentryData(NULL);

    if (NULL == pInfo) {
        pTextFieldInfo = midpMalloc(sizeof(midp_textfield_info));
        if (NULL == pTextFieldInfo) {
            result = LFJ_TEXTFIELD_ERROR;
            goto cleanup;
        }
        caretPos = KNI_GetIntField(TextCursorObj, KNI_GetFieldID(TextCursorClass, "index", "I"));
        maxLength = KNI_GetArrayLength(bufferJCharArray);
        currentLength = KNI_GetIntField(dynamicCharacterArrayObj, 
        KNI_GetFieldID(dynamicCharacterArrayClass, "length", "I"));
        text = midpNewStringFromArray(bufferJCharArray, maxLength);
        inputMode = midpNewString(inputModeJString);
        title = midpNewString(titleJString);
        if (text.len < NULL_LEN || inputMode.len < NULL_LEN || title.len< NULL_LEN){
            result = LFJ_TEXTFIELD_ERROR;
            goto cleanup;
        }

        /* Setup context */
        pTextFieldInfo->text = text;
        pTextFieldInfo->textLength = currentLength;
        pTextFieldInfo->caretPos = caretPos;

        /*result = pdLaunchNativeTextField(text.data, &pTextFieldInfo->textLength, maxLength, 
            &pTextFieldInfo->caretPos, title.data, title.len, inputMode.data, inputMode.len, 
            constraint & MIDP_CONSTRAINT_MASK, constraint & MIDP_MODIFIER_MASK, 
            keyCode);*/
        result = javacall_input_textfield_launch(text.data, &pTextFieldInfo->textLength, maxLength,
                &pTextFieldInfo->caretPos, title.data, title.len, inputMode.data, inputMode.len, 
                constraint & LFJ_CONSTRAINT_MASK, constraint & LFJ_MODIFIER_MASK, 
                keyCode);
    
        if (LFJ_TEXTFIELD_COMMIT == result) { 
           KNI_SetRawArrayRegion(bufferJCharArray, 0, maxLength* sizeof (jchar), (jbyte *)text.data);
           KNI_SetIntField(dynamicCharacterArrayObj, KNI_GetFieldID(dynamicCharacterArrayClass,
               "length", "I"), pTextFieldInfo->textLength);
           KNI_SetIntField(TextCursorObj, KNI_GetFieldID(TextCursorClass, "index", "I"), pTextFieldInfo->caretPos);

           midpFree(pTextFieldInfo);
           MIDP_FREE_STRING(title);
           MIDP_FREE_STRING(text);
        
        } else if (LFJ_TEXTFIELD_WOULDBLOCK == result) {
            pInfo = (MidpReentryData*)(SNI_AllocateReentryData(sizeof(MidpReentryData)));
            if (NULL != pInfo) {
                pInfo->descriptor = 0;  /* FIXME - There is no descriptor now */
                pInfo->waitingFor = TEXT_INPUT_SIGNAL;
                pInfo->pResult = (void*)pTextFieldInfo;
                needBlocking = KNI_TRUE;
            }
        }
        MIDP_FREE_STRING(title);
        MIDP_FREE_STRING(inputMode);

    } else {
        pTextFieldInfo = (midp_textfield_info*)pInfo->pResult;
        result = pInfo->status;
        /*result = javacall_input_textfield_launch(pTextFieldInfo->text.data, &pTextFieldInfo->textLength, pTextFieldInfo->textLength,
             &pTextFieldInfo->caretPos, title.data, title.len, inputMode.data, inputMode.len, 
             constraint & LFJ_CONSTRAINT_MASK, constraint & LFJ_MODIFIER_MASK, 
             keyCode);*/

        KNI_SetRawArrayRegion(bufferJCharArray, 0, pTextFieldInfo->textLength * sizeof (jchar), 
                              (jbyte *)pTextFieldInfo->text.data);
        KNI_SetIntField(dynamicCharacterArrayObj, KNI_GetFieldID(dynamicCharacterArrayClass,
                        "length", "I"), pTextFieldInfo->textLength);
        KNI_SetIntField(TextCursorObj, KNI_GetFieldID(TextCursorClass, "index", "I"), pTextFieldInfo->caretPos);

        MIDP_FREE_STRING(pTextFieldInfo->text);
        midpFree(pTextFieldInfo);
        /* FIXME - Enable LCD flush from JVM */
    }

cleanup:
    
    KNI_EndHandles();
    
    if (LFJ_TEXTFIELD_ERROR == result) {
        KNI_ThrowNew(midpRuntimeException, "Exception while launching native text editor");
    }
    
    if (KNI_TRUE == needBlocking) {
        /* FIXME - Disable LCD flush from JVM */
        SNI_BlockThread();
    }

    KNI_ReturnVoid();
}                  