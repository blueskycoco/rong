#include "sal/sal_pres.h"
#include "sal_impl.h"
//request pres
int sal_request_presentation(SalOp *op){
	char info_body[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			" <media_control>\n"
			"	<presentation_token_control>\n"
			"		<vc_primitive>\n"
			"			<pres_token_request>\n"
			"			</pres_token_request>\n"
			"		</vc_primitive>\n"
			"	</presentation_token_control>\n"
			"</media_control>\n";
	size_t content_lenth = sizeof(info_body) - 1;
	belle_sip_dialog_state_t dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL; /*no dialog = dialog in NULL state*/
	if (dialog_state == BELLE_SIP_DIALOG_CONFIRMED) {
		belle_sip_request_t* info =	belle_sip_dialog_create_queued_request(op->dialog,"INFO");
		int error=TRUE;
		if (info) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_type_create("application","h239_control+xml")));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_lenth)));
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(info),info_body,content_lenth);
			error=sal_op_send_request(op,info);
		}
		if (error)
			ms_warning("Cannot send pres request to [%s] ", sal_op_get_to(op));

	} else {
		ms_warning("Cannot send pres request to [%s] because dialog [%p] in wrong state [%s]",sal_op_get_to(op)
																							,op->dialog
																							,belle_sip_dialog_state_to_string(dialog_state));
	}

	return 0;
}

//request pres response
int sal_presentation_response(SalOp *op,int accepted){
	char info_body_true[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			" <media_control>\n"
			"	<presentation_token_control>\n"
			"		<vc_primitive>\n"
			"			<pres_token_response>\n"
			"			<accepted status=\"true\"/>\n"
			"			</pres_token_response>\n"
			"		</vc_primitive>\n"
			"	</presentation_token_control>\n"
			"</media_control>\n";
	char info_body_false[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			" <media_control>\n"
			"	<presentation_token_control>\n"
			"		<vc_primitive>\n"
			"			<pres_token_response>\n"
			"			<accepted status=\"true\"/>\n"
			"			</pres_token_response>\n"
			"		</vc_primitive>\n"
			"	</presentation_token_control>\n"
			"</media_control>\n";
	size_t content_lenth = accepted?sizeof(info_body_true)- 1:sizeof(info_body_false) - 1;
	belle_sip_dialog_state_t dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL; /*no dialog = dialog in NULL state*/
	if (dialog_state == BELLE_SIP_DIALOG_CONFIRMED) {
		belle_sip_request_t* info =	belle_sip_dialog_create_queued_request(op->dialog,"INFO");
		int error=TRUE;
		if (info) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_type_create("application","h239_control+xml")));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_lenth)));
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(info),accepted?info_body_true:info_body_false,content_lenth);
			error=sal_op_send_request(op,info);
		}
		if (error)
			ms_warning("Cannot send pres response to [%s] ", sal_op_get_to(op));

	} else {
		ms_warning("Cannot send pres response to [%s] because dialog [%p] in wrong state [%s]",sal_op_get_to(op)
																							,op->dialog
																							,belle_sip_dialog_state_to_string(dialog_state));
	}

	return 0;
}
//release presentation
int sal_release_presentation(SalOp *op){
	char info_body[] =
			"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
			" <media_control>\n"
			"	<presentation_token_control>\n"
			"		<vc_primitive>\n"
			"			<pres_token_release>\n"
			"			</pres_token_release>\n"
			"		</vc_primitive>\n"
			"	</presentation_token_control>\n"
			"</media_control>\n";
	size_t content_lenth = sizeof(info_body) - 1;
	belle_sip_dialog_state_t dialog_state=op->dialog?belle_sip_dialog_get_state(op->dialog):BELLE_SIP_DIALOG_NULL; /*no dialog = dialog in NULL state*/
	if (dialog_state == BELLE_SIP_DIALOG_CONFIRMED) {
		belle_sip_request_t* info =	belle_sip_dialog_create_queued_request(op->dialog,"INFO");
		int error=TRUE;
		if (info) {
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_type_create("application","h239_control+xml")));
			belle_sip_message_add_header(BELLE_SIP_MESSAGE(info),BELLE_SIP_HEADER(belle_sip_header_content_length_create(content_lenth)));
			belle_sip_message_set_body(BELLE_SIP_MESSAGE(info),info_body,content_lenth);
			error=sal_op_send_request(op,info);
		}
		if (error)
			ms_warning("Cannot send pres request to [%s] ", sal_op_get_to(op));

	} else {
		ms_warning("Cannot send pres request to [%s] because dialog [%p] in wrong state [%s]",sal_op_get_to(op)
																							,op->dialog
																							,belle_sip_dialog_state_to_string(dialog_state));
	}

	return 0;
}

