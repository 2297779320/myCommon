/**************************************************************************
* 版    权： 	Copyright (c) 2018
* 文件名称：	JsonMsgDispatch.h
* 文件标识： 
* 内容摘要： JSON消息处理头文件
* 其它说明： 
* 当前版本：	V1.0
* 作    者	    ：	
* 完成日期：	2019年01月 23日
*
* 修改记录1	：
*	修改日期：
*	版 本 号：
*	修 改 人：
*	修改内容：
**************************************************************************/

#ifndef _H_JSONMSGDISPATCH_
#define _H_JSONMSGDISPATCH_

#ifdef		__cplusplus
extern		"C"
{
#endif

/**************************************************************************
 *                         头文件引用                                     *
 **************************************************************************/
#include "TlPublic.h"

#include "typedef.h"

/**************************************************************************
 *                        常量定义                                   *
 **************************************************************************/
#define JSON_METHOD_GET						"$get"
#define JSON_METHOD_SET						"$set"
#define JSON_METHOD_INIT						"$init"
#define JSON_METHOD_ADD						"$add"
#define JSON_METHOD_DEL						"$del"
#define JSON_METHOD_DELALL					"$delAll"
#define JSON_METHOD_UPDATE					"$update"
#define JSON_METHOD_QUERY						"$query"
#define JSON_TYPE_REQUEST						"$request"
#define JSON_TYPE_RESPONSE						"$report"

/* {prefix}.{type:$data|method}.{clientId}.{domainId}.{devId}.{moduleId}.v{x}.{funId}*/
#define TOPIC_PART_CNT							8

#define TOPIC_PART_PREFIX						0
#define TOPIC_PART_TYPEMETHOD					1
#define TOPIC_PART_CLIENT						2
#define TOPIC_PART_DOMIN						3
#define TOPIC_PART_DEV							4
#define TOPIC_PART_MODULE						5
#define TOPIC_PART_VERSION						6
#define TOPIC_PART_FUNCID						7

/**************************************************************************
 *                       数据类型                                   *
 **************************************************************************/
typedef E_StateCode (*JsonMsgProcess)(void *pPrivate, INT8 *strTopic, INT8 *strJson, INT8 *strMsg, INT8 **pstrJsonResult);
typedef struct
{
	const INT8		*pcTopic;
	JsonMsgProcess	pfProcess;
}T_JsonMsgDispatch;

 typedef struct
{
	INT8		*pcAddr;
	UINT32	uiLen;
}T_MultiChar;

typedef struct
{
	T_MultiChar	atParam[TOPIC_PART_CNT];
}T_UserTopic;

/**************************************************************************
 *                        全局函数                                *
 **************************************************************************/
BOOL IsFuncIdPartEqual(INT8 *strFmt, INT8 *strFuncId);
BOOL IsFuncIdEqual(INT8 *strFmt, INT8 *strFuncId);
E_StateCode ParseFuncId(INT8 *strFmt, INT8 *strFuncId, UINT32 *puiIdx);
E_StateCode MakeFuncId(INT8 *strFmt, INT8 *strFuncId, UINT32 *puiIdx);
E_StateCode ParseFmtTopic(INT8 *strFmt, T_UserTopic *ptTopic);
E_StateCode ParseUserTopic(INT8 *strTopic, T_UserTopic *ptTopic);
E_StateCode MakeUserTopic(INT8 *strTopic, T_UserTopic *ptTopic);
BOOL IsMultiCharEqual(T_MultiChar *ptMChar1, T_MultiChar *ptMChar2);
BOOL IsMultiCharEqualString(T_MultiChar *ptMChar1, INT8 *strValue);
BOOL IsTopicEqual(INT8 *strFmt, INT8 *strTopic);
BOOL IsTopicEqual2(INT8 *strFmt, T_UserTopic *ptUserTopic);
T_JsonMsgDispatch *FindMsgDispathByUserTopic(T_JsonMsgDispatch *ptTable, T_UserTopic *ptTopic);

 /**********************************************************************
* 函数名称：FindJsonMsgDispatch
* 功能描述：查找JSON消息处理项
* 输入参数：ptTable - 处理表
					strTopic - 消息主题
* 输出参数：无
* 返 回 值：	 T_JsonMsgDispatch *
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
T_JsonMsgDispatch *FindJsonMsgDispatch(T_JsonMsgDispatch *ptTable, INT8 *strTopic);

 /**********************************************************************
* 函数名称：FindJsonMsgDispatch
* 功能描述：查找JSON消息处理项
* 输入参数：strTopicFmt - 消息格式，支持通配符'*'、%d
					strTopic - 消息主题
				strPrefix strType strDomin strDevId strModuleId uiVersion puiTopicIndex -
														主题参数，可以为NULL
* 输出参数：无
* 返 回 值：	E_StateCode
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
E_StateCode MakeJsonMsgTopic(INT8 *strTopicFmt, 
							INT8 *strTopic,
							INT8 *strPrefix, 
							INT8 *strType, 
							INT8 *strDomin, 
							INT8 *strDevId,
							INT8 *strModuleId,
							UINT32 uiVersion,
							UINT32 *puiTopicIndex);

/**********************************************************************
* 函数名称：GetTopicId
* 功能描述：从TOPIC中获取TOPIC ID
* 输入参数：strTopic - 
* 输出参数：
* 返 回 值：	TOPIC ID
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
INT8 *GetTopicId(INT8 *strTopic);

/**********************************************************************
* 函数名称：GetClientId
* 功能描述：从TOPIC中获取客户ID
* 输入参数：strTopic - 
				uiMaxLen - 客户ID的最大长度
* 输出参数：strClientId - 
* 返 回 值：	TOPIC ID
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
E_StateCode GetClientId(INT8 *strTopic, INT8 *strClientId, UINT32 uiMaxLen);

/**********************************************************************
* 函数名称：GetClientIdNumernic
* 功能描述：从TOPIC中获取客户ID,以数字形式返回
* 输入参数：strTopic - 
* 输出参数：
* 返 回 值：	TOPIC ID
* 其它说明： 
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/03/11	     V1.0	           
***********************************************************************/
UINT32 GetClientIdNumernic(INT8 *strTopic);

/**********************************************************************
* 函数名称：SetJsonMsgLocalModuleName
* 功能描述：设置本地模块名
* 输入参数：strLocalModuleName 
* 输出参数：无
* 返 回 值：	E_StateCode
* 其它说明：
* 修改日期        版本号     修改人	      修改内容
* -----------------------------------------------
* 2019/01/06    V1.0	tanrp	           
***********************************************************************/
E_StateCode SetJsonMsgLocalModuleName(INT8 *strLocalModuleName);
							
#ifdef		__cplusplus
}
#endif

#endif
	

