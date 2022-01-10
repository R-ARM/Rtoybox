struct dk_message
{
	uint32_t opcode;
	uint64_t data;
	uint64_t data_alt;
	uint64_t data_alt_alt;
};

#define R_MSG_SPIERDALAJ	0
#define R_MSG_INPUT		1
#define R_MSG_LOG		2
#define R_MSG_FOO		3

#define R_M_LOG_ERR		0
#define R_M_LOG_WARN		1
#define R_M_LOG_INFO		2
#define R_M_LOG_DEBUG		3

int receive_dk_msg(int sockfd, struct dk_message *msg_in)
{
	if (sockfd > 0)
		return read(sockfd, msg_in, sizeof(struct dk_message));
	else
	{
		sleep(1);
		msg_in->opcode = R_MSG_FOO;
		return 0;
	}
}

int send_dk_msg(int sockfd, uint32_t opcode, uint32_t data, uint32_t data_alt, uint32_t data_alt_alt)
{
	if (sockfd > 0)
	{
		struct dk_message message;
		message.opcode = opcode;
		message.data = data;
		message.data_alt = data_alt;
		message.data_alt_alt = data_alt_alt;
		return write(sockfd, &message, sizeof(struct dk_message));
	}
	else
		return 0;
}

int quit_gracefully(int sockfd)
{
	send_dk_msg(sockfd, R_MSG_SPIERDALAJ, 0, 0, 0);
}
