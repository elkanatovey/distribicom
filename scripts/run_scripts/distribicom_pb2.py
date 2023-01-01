# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: distribicom.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\x11\x64istribicom.proto\x12\x0b\x64istribicom\"\x16\n\x03\x41\x63k\x12\x0f\n\x07success\x18\x01 \x01(\x08\"\xb2\x01\n\x0eWorkerTaskPart\x12-\n\nmatrixPart\x18\x01 \x01(\x0b\x32\x17.distribicom.MatrixPartH\x00\x12\'\n\x04gkey\x18\x03 \x01(\x0b\x32\x17.distribicom.GaloisKeysH\x00\x12\'\n\x02md\x18\x04 \x01(\x0b\x32\x19.distribicom.TaskMetadataH\x00\x12\x17\n\rtask_complete\x18\x05 \x01(\x08H\x00\x42\x06\n\x04part\",\n\x0cTaskMetadata\x12\r\n\x05round\x18\x01 \x01(\x04\x12\r\n\x05\x65poch\x18\x02 \x01(\x04\"}\n\nMatrixPart\x12&\n\x03\x63tx\x18\x01 \x01(\x0b\x32\x17.distribicom.CiphertextH\x00\x12%\n\x03ptx\x18\x02 \x01(\x0b\x32\x16.distribicom.PlaintextH\x00\x12\x0b\n\x03row\x18\x03 \x01(\r\x12\x0b\n\x03\x63ol\x18\x04 \x01(\rB\x06\n\x04\x64\x61ta\"\x1f\n\x0fQueryCiphertext\x12\x0c\n\x04\x64\x61ta\x18\x01 \x01(\x0c\"+\n\nGaloisKeys\x12\x0c\n\x04keys\x18\x01 \x01(\x0c\x12\x0f\n\x07key_pos\x18\x02 \x01(\x04\"\x1a\n\nCiphertext\x12\x0c\n\x04\x64\x61ta\x18\x01 \x01(\x0c\"4\n\x0b\x43iphertexts\x12%\n\x04\x64\x61ta\x18\x01 \x03(\x0b\x32\x17.distribicom.Ciphertext\"\x19\n\tPlaintext\x12\x0c\n\x04\x64\x61ta\x18\x01 \x01(\x0c\"2\n\nPlaintexts\x12$\n\x04\x64\x61ta\x18\x01 \x03(\x0b\x32\x16.distribicom.Plaintext\"C\n\x0cWriteRequest\x12\x11\n\tptxnumber\x18\x01 \x01(\x04\x12\x12\n\nwhereinptx\x18\x02 \x01(\r\x12\x0c\n\x04\x64\x61ta\x18\x03 \x01(\x0c\"4\n\x0bPirResponse\x12%\n\x04\x64\x61ta\x18\x01 \x03(\x0b\x32\x17.distribicom.Ciphertext\"\x82\x01\n\x12\x43lientQueryRequest\x12+\n\nquery_dim1\x18\x01 \x03(\x0b\x32\x17.distribicom.Ciphertext\x12+\n\nquery_dim2\x18\x02 \x03(\x0b\x32\x17.distribicom.Ciphertext\x12\x12\n\nmailbox_id\x18\x03 \x01(\r\"V\n\x15\x43lientRegistryRequest\x12\x13\n\x0b\x63redentials\x18\x01 \x01(\x0c\x12\x13\n\x0b\x63lient_port\x18\x02 \x01(\r\x12\x13\n\x0bgalois_keys\x18\x03 \x01(\x0c\"U\n\x13\x43lientRegistryReply\x12\x13\n\x0b\x63redentials\x18\x01 \x01(\x0c\x12\x15\n\rnum_mailboxes\x18\x02 \x01(\r\x12\x12\n\nmailbox_id\x18\x03 \x01(\r\"R\n\rClientConfigs\x12,\n\x0b\x61pp_configs\x18\x01 \x01(\x0b\x32\x17.distribicom.AppConfigs\x12\x13\n\x0b\x63lient_port\x18\x02 \x01(\r\"$\n\x13TellNewRoundRequest\x12\r\n\x05round\x18\x01 \x01(\r\"S\n\x15WorkerRegistryRequest\x12\x13\n\x0b\x63redentials\x18\x01 \x01(\x0c\x12\x12\n\nworkerPort\x18\x02 \x01(\r\x12\x11\n\tworker_ip\x18\x03 \x01(\t\"C\n\nSubscriber\x12\x11\n\tsecretKey\x18\x01 \x01(\x0c\x12\x11\n\tpublicKey\x18\x02 \x01(\x0c\x12\x0f\n\x07sym_key\x18\x03 \x01(\x0c\"h\n\tQueryInfo\x12\x16\n\x0e\x63lient_mailbox\x18\x01 \x01(\r\x12\x13\n\x0bgalois_keys\x18\x02 \x01(\x0c\x12.\n\x05query\x18\x03 \x01(\x0b\x32\x1f.distribicom.ClientQueryRequest\"\x9b\x02\n\x07\x43onfigs\x12\x0e\n\x06scheme\x18\x01 \x01(\t\x12\x19\n\x11polynomial_degree\x18\x02 \x01(\r\x12\'\n\x1flogarithm_plaintext_coefficient\x18\x03 \x01(\r\x12\x0f\n\x07\x64\x62_rows\x18\x04 \x01(\r\x12\x0f\n\x07\x64\x62_cols\x18\x05 \x01(\r\x12\x18\n\x10size_per_element\x18\x06 \x01(\r\x12\x12\n\ndimensions\x18\x07 \x01(\r\x12\x1a\n\x12number_of_elements\x18\x08 \x01(\r\x12\x15\n\ruse_symmetric\x18\t \x01(\x08\x12\x14\n\x0cuse_batching\x18\n \x01(\x08\x12#\n\x1buse_recursive_mod_switching\x18\x0b \x01(\x08\"\xd3\x01\n\nAppConfigs\x12%\n\x07\x63onfigs\x18\x01 \x01(\x0b\x32\x14.distribicom.Configs\x12\x1c\n\x14main_server_hostname\x18\x02 \x01(\t\x12\x18\n\x10main_server_cert\x18\x03 \x01(\x0c\x12\x19\n\x11number_of_workers\x18\x04 \x01(\x04\x12\x17\n\x0fquery_wait_time\x18\x05 \x01(\x04\x12\x19\n\x11number_of_clients\x18\x06 \x01(\x04\x12\x17\n\x0fworker_num_cpus\x18\x07 \x01(\r2G\n\x06Worker\x12=\n\x08SendTask\x12\x1b.distribicom.WorkerTaskPart\x1a\x10.distribicom.Ack\"\x00(\x01\x32\xa4\x01\n\x07Manager\x12W\n\x10RegisterAsWorker\x12\".distribicom.WorkerRegistryRequest\x1a\x1b.distribicom.WorkerTaskPart\"\x00\x30\x01\x12@\n\x0fReturnLocalWork\x12\x17.distribicom.MatrixPart\x1a\x10.distribicom.Ack\"\x00(\x01\x32\x8f\x01\n\x06\x43lient\x12\x36\n\x06\x41nswer\x12\x18.distribicom.PirResponse\x1a\x10.distribicom.Ack\"\x00\x12M\n\x0cTellNewRound\x12 .distribicom.TellNewRoundRequest\x1a\x19.distribicom.WriteRequest\"\x00\x32\xe3\x01\n\x06Server\x12Z\n\x10RegisterAsClient\x12\".distribicom.ClientRegistryRequest\x1a .distribicom.ClientRegistryReply\"\x00\x12\x41\n\nStoreQuery\x12\x1f.distribicom.ClientQueryRequest\x1a\x10.distribicom.Ack\"\x00\x12:\n\tWriteToDB\x12\x19.distribicom.WriteRequest\x1a\x10.distribicom.Ack\"\x00\x62\x06proto3')



_ACK = DESCRIPTOR.message_types_by_name['Ack']
_WORKERTASKPART = DESCRIPTOR.message_types_by_name['WorkerTaskPart']
_TASKMETADATA = DESCRIPTOR.message_types_by_name['TaskMetadata']
_MATRIXPART = DESCRIPTOR.message_types_by_name['MatrixPart']
_QUERYCIPHERTEXT = DESCRIPTOR.message_types_by_name['QueryCiphertext']
_GALOISKEYS = DESCRIPTOR.message_types_by_name['GaloisKeys']
_CIPHERTEXT = DESCRIPTOR.message_types_by_name['Ciphertext']
_CIPHERTEXTS = DESCRIPTOR.message_types_by_name['Ciphertexts']
_PLAINTEXT = DESCRIPTOR.message_types_by_name['Plaintext']
_PLAINTEXTS = DESCRIPTOR.message_types_by_name['Plaintexts']
_WRITEREQUEST = DESCRIPTOR.message_types_by_name['WriteRequest']
_PIRRESPONSE = DESCRIPTOR.message_types_by_name['PirResponse']
_CLIENTQUERYREQUEST = DESCRIPTOR.message_types_by_name['ClientQueryRequest']
_CLIENTREGISTRYREQUEST = DESCRIPTOR.message_types_by_name['ClientRegistryRequest']
_CLIENTREGISTRYREPLY = DESCRIPTOR.message_types_by_name['ClientRegistryReply']
_CLIENTCONFIGS = DESCRIPTOR.message_types_by_name['ClientConfigs']
_TELLNEWROUNDREQUEST = DESCRIPTOR.message_types_by_name['TellNewRoundRequest']
_WORKERREGISTRYREQUEST = DESCRIPTOR.message_types_by_name['WorkerRegistryRequest']
_SUBSCRIBER = DESCRIPTOR.message_types_by_name['Subscriber']
_QUERYINFO = DESCRIPTOR.message_types_by_name['QueryInfo']
_CONFIGS = DESCRIPTOR.message_types_by_name['Configs']
_APPCONFIGS = DESCRIPTOR.message_types_by_name['AppConfigs']
Ack = _reflection.GeneratedProtocolMessageType('Ack', (_message.Message,), {
  'DESCRIPTOR' : _ACK,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Ack)
  })
_sym_db.RegisterMessage(Ack)

WorkerTaskPart = _reflection.GeneratedProtocolMessageType('WorkerTaskPart', (_message.Message,), {
  'DESCRIPTOR' : _WORKERTASKPART,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.WorkerTaskPart)
  })
_sym_db.RegisterMessage(WorkerTaskPart)

TaskMetadata = _reflection.GeneratedProtocolMessageType('TaskMetadata', (_message.Message,), {
  'DESCRIPTOR' : _TASKMETADATA,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.TaskMetadata)
  })
_sym_db.RegisterMessage(TaskMetadata)

MatrixPart = _reflection.GeneratedProtocolMessageType('MatrixPart', (_message.Message,), {
  'DESCRIPTOR' : _MATRIXPART,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.MatrixPart)
  })
_sym_db.RegisterMessage(MatrixPart)

QueryCiphertext = _reflection.GeneratedProtocolMessageType('QueryCiphertext', (_message.Message,), {
  'DESCRIPTOR' : _QUERYCIPHERTEXT,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.QueryCiphertext)
  })
_sym_db.RegisterMessage(QueryCiphertext)

GaloisKeys = _reflection.GeneratedProtocolMessageType('GaloisKeys', (_message.Message,), {
  'DESCRIPTOR' : _GALOISKEYS,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.GaloisKeys)
  })
_sym_db.RegisterMessage(GaloisKeys)

Ciphertext = _reflection.GeneratedProtocolMessageType('Ciphertext', (_message.Message,), {
  'DESCRIPTOR' : _CIPHERTEXT,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Ciphertext)
  })
_sym_db.RegisterMessage(Ciphertext)

Ciphertexts = _reflection.GeneratedProtocolMessageType('Ciphertexts', (_message.Message,), {
  'DESCRIPTOR' : _CIPHERTEXTS,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Ciphertexts)
  })
_sym_db.RegisterMessage(Ciphertexts)

Plaintext = _reflection.GeneratedProtocolMessageType('Plaintext', (_message.Message,), {
  'DESCRIPTOR' : _PLAINTEXT,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Plaintext)
  })
_sym_db.RegisterMessage(Plaintext)

Plaintexts = _reflection.GeneratedProtocolMessageType('Plaintexts', (_message.Message,), {
  'DESCRIPTOR' : _PLAINTEXTS,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Plaintexts)
  })
_sym_db.RegisterMessage(Plaintexts)

WriteRequest = _reflection.GeneratedProtocolMessageType('WriteRequest', (_message.Message,), {
  'DESCRIPTOR' : _WRITEREQUEST,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.WriteRequest)
  })
_sym_db.RegisterMessage(WriteRequest)

PirResponse = _reflection.GeneratedProtocolMessageType('PirResponse', (_message.Message,), {
  'DESCRIPTOR' : _PIRRESPONSE,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.PirResponse)
  })
_sym_db.RegisterMessage(PirResponse)

ClientQueryRequest = _reflection.GeneratedProtocolMessageType('ClientQueryRequest', (_message.Message,), {
  'DESCRIPTOR' : _CLIENTQUERYREQUEST,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.ClientQueryRequest)
  })
_sym_db.RegisterMessage(ClientQueryRequest)

ClientRegistryRequest = _reflection.GeneratedProtocolMessageType('ClientRegistryRequest', (_message.Message,), {
  'DESCRIPTOR' : _CLIENTREGISTRYREQUEST,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.ClientRegistryRequest)
  })
_sym_db.RegisterMessage(ClientRegistryRequest)

ClientRegistryReply = _reflection.GeneratedProtocolMessageType('ClientRegistryReply', (_message.Message,), {
  'DESCRIPTOR' : _CLIENTREGISTRYREPLY,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.ClientRegistryReply)
  })
_sym_db.RegisterMessage(ClientRegistryReply)

ClientConfigs = _reflection.GeneratedProtocolMessageType('ClientConfigs', (_message.Message,), {
  'DESCRIPTOR' : _CLIENTCONFIGS,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.ClientConfigs)
  })
_sym_db.RegisterMessage(ClientConfigs)

TellNewRoundRequest = _reflection.GeneratedProtocolMessageType('TellNewRoundRequest', (_message.Message,), {
  'DESCRIPTOR' : _TELLNEWROUNDREQUEST,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.TellNewRoundRequest)
  })
_sym_db.RegisterMessage(TellNewRoundRequest)

WorkerRegistryRequest = _reflection.GeneratedProtocolMessageType('WorkerRegistryRequest', (_message.Message,), {
  'DESCRIPTOR' : _WORKERREGISTRYREQUEST,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.WorkerRegistryRequest)
  })
_sym_db.RegisterMessage(WorkerRegistryRequest)

Subscriber = _reflection.GeneratedProtocolMessageType('Subscriber', (_message.Message,), {
  'DESCRIPTOR' : _SUBSCRIBER,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Subscriber)
  })
_sym_db.RegisterMessage(Subscriber)

QueryInfo = _reflection.GeneratedProtocolMessageType('QueryInfo', (_message.Message,), {
  'DESCRIPTOR' : _QUERYINFO,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.QueryInfo)
  })
_sym_db.RegisterMessage(QueryInfo)

Configs = _reflection.GeneratedProtocolMessageType('Configs', (_message.Message,), {
  'DESCRIPTOR' : _CONFIGS,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.Configs)
  })
_sym_db.RegisterMessage(Configs)

AppConfigs = _reflection.GeneratedProtocolMessageType('AppConfigs', (_message.Message,), {
  'DESCRIPTOR' : _APPCONFIGS,
  '__module__' : 'distribicom_pb2'
  # @@protoc_insertion_point(class_scope:distribicom.AppConfigs)
  })
_sym_db.RegisterMessage(AppConfigs)

_WORKER = DESCRIPTOR.services_by_name['Worker']
_MANAGER = DESCRIPTOR.services_by_name['Manager']
_CLIENT = DESCRIPTOR.services_by_name['Client']
_SERVER = DESCRIPTOR.services_by_name['Server']
if _descriptor._USE_C_DESCRIPTORS == False:

  DESCRIPTOR._options = None
  _ACK._serialized_start=34
  _ACK._serialized_end=56
  _WORKERTASKPART._serialized_start=59
  _WORKERTASKPART._serialized_end=237
  _TASKMETADATA._serialized_start=239
  _TASKMETADATA._serialized_end=283
  _MATRIXPART._serialized_start=285
  _MATRIXPART._serialized_end=410
  _QUERYCIPHERTEXT._serialized_start=412
  _QUERYCIPHERTEXT._serialized_end=443
  _GALOISKEYS._serialized_start=445
  _GALOISKEYS._serialized_end=488
  _CIPHERTEXT._serialized_start=490
  _CIPHERTEXT._serialized_end=516
  _CIPHERTEXTS._serialized_start=518
  _CIPHERTEXTS._serialized_end=570
  _PLAINTEXT._serialized_start=572
  _PLAINTEXT._serialized_end=597
  _PLAINTEXTS._serialized_start=599
  _PLAINTEXTS._serialized_end=649
  _WRITEREQUEST._serialized_start=651
  _WRITEREQUEST._serialized_end=718
  _PIRRESPONSE._serialized_start=720
  _PIRRESPONSE._serialized_end=772
  _CLIENTQUERYREQUEST._serialized_start=775
  _CLIENTQUERYREQUEST._serialized_end=905
  _CLIENTREGISTRYREQUEST._serialized_start=907
  _CLIENTREGISTRYREQUEST._serialized_end=993
  _CLIENTREGISTRYREPLY._serialized_start=995
  _CLIENTREGISTRYREPLY._serialized_end=1080
  _CLIENTCONFIGS._serialized_start=1082
  _CLIENTCONFIGS._serialized_end=1164
  _TELLNEWROUNDREQUEST._serialized_start=1166
  _TELLNEWROUNDREQUEST._serialized_end=1202
  _WORKERREGISTRYREQUEST._serialized_start=1204
  _WORKERREGISTRYREQUEST._serialized_end=1287
  _SUBSCRIBER._serialized_start=1289
  _SUBSCRIBER._serialized_end=1356
  _QUERYINFO._serialized_start=1358
  _QUERYINFO._serialized_end=1462
  _CONFIGS._serialized_start=1465
  _CONFIGS._serialized_end=1748
  _APPCONFIGS._serialized_start=1751
  _APPCONFIGS._serialized_end=1962
  _WORKER._serialized_start=1964
  _WORKER._serialized_end=2035
  _MANAGER._serialized_start=2038
  _MANAGER._serialized_end=2202
  _CLIENT._serialized_start=2205
  _CLIENT._serialized_end=2348
  _SERVER._serialized_start=2351
  _SERVER._serialized_end=2578
# @@protoc_insertion_point(module_scope)
