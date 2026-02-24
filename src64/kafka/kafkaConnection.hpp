#include <iostream>
#include <unistd.h>
#include <sstream>
#include <librdkafka/rdkafka.h>
#include <librdkafka/rdkafkacpp.h>

class kafkaProducer {

	rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
	rd_kafka_t *rk;
	rd_kafka_topic_t *rkt;

public:
	void init(std::string const & username, std::string const & password, std::string const & certificate, std::string const & topic, std::string const & host, unsigned port) {
		char hostname[128];
		char errstr[512];
		
		rd_kafka_conf_t *conf = rd_kafka_conf_new();
		
		if (gethostname(hostname, sizeof(hostname))) {
		  fprintf(stderr, "%% Failed to lookup hostname\n");
		  exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "client.id", hostname,
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		std::stringstream ss;
		ss << host << ":" << port;
		if (rd_kafka_conf_set(conf, "bootstrap.servers", ss.str().c_str(),
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "security.protocol", "SASL_SSL",
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "sasl.mechanism", "PLAIN",
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		//if (rd_kafka_conf_set(conf, "sasl.jaas.config", "org.apache.kafka.common.security.plain.PlainLoginModule required",
		//                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		//  fprintf(stderr, "%% %s\n", errstr);
		//  exit(1);
		//}
		
		if (rd_kafka_conf_set(conf, "sasl.username", username.c_str(),
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "sasl.password", password.c_str(),
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "ssl.ca.location", certificate.c_str(),
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		if (rd_kafka_topic_conf_set(topic_conf, "acks", "all",
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		/* Create Kafka producer handle */
		if (!(rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf,
		                        errstr, sizeof(errstr)))) {
		  fprintf(stderr, "%% Failed to create new producer: %s\n", errstr);
		  exit(1);
		}
		rkt = rd_kafka_topic_new(rk, topic.c_str(), topic_conf);
	}

	bool asyncWrite(void *payload, size_t payload_len, const void *key, size_t key_len) {
		if (rd_kafka_produce(rkt, RD_KAFKA_PARTITION_UA,
		                     RD_KAFKA_MSG_F_COPY,
		                     payload, payload_len,
		                     key, key_len,
		                     NULL
		                    ) == -1
		) {
		  fprintf(stderr, "%% Failed to produce to topic: %s\n",
		                  rd_kafka_err2str(rd_kafka_last_error())
		         );
		  return false;
		}
		//std::cout << "produce " << (char *)key << "\t" << (char *)payload << std::endl;
		//static void on_delivery(rd_kafka_t *rk,
		//                        const rd_kafka_message_t *rkmessage
		//                        void *opaque) {
		//  if (rkmessage->err)
		//    fprintf(stderr, "%% Message delivery failed: %s\n",
		//            rd_kafka_message_errstr(rkmessage));
		//}
		//
		//void init_rd_kafka() {
		//  rd_kafka_conf_t *conf = rd_kafka_conf_new();
		//  rd_kafka_conf_set_dr_msg_cb(conf, on_delivery);
		//
		//  // initialization omitted
		//}
		return true;
	}


};


class kafkaConsumer {
	rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
	rd_kafka_t *rk;
public:
	void init(std::string const & group, std::string const & host, unsigned port) {
		char hostname[128];
		char errstr[512];
		
		rd_kafka_conf_t *conf = rd_kafka_conf_new();
		
		if (gethostname(hostname, sizeof(hostname))) {
		 fprintf(stderr, "%% Failed to lookup hostname\n");
		 exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "client.id", hostname,
		                     errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		 fprintf(stderr, "%% %s\n", errstr);
		 exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "group.id", group.c_str(),
		                     errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		 fprintf(stderr, "%% %s\n", errstr);
		 exit(1);
		}
		
		if (rd_kafka_conf_set(conf, "enable.partition.eof", "true",
		                     errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		 fprintf(stderr, "%% %s\n", errstr);
		 exit(1);
		}
		std::stringstream ss;
		ss << host << ":" << port;
		if (rd_kafka_conf_set(conf, "bootstrap.servers", ss.str().c_str(),
		                     errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		 fprintf(stderr, "%% %s\n", errstr);
		 exit(1);
		}

		if (rd_kafka_topic_conf_set(topic_conf, "acks", "all",
		                      errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
		  fprintf(stderr, "%% %s\n", errstr);
		  exit(1);
		}
		
		/* Create Kafka consumer handle */
		if (!(rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf,
		                       errstr, sizeof(errstr)))) {
		 fprintf(stderr, "%% Failed to create new consumer: %s\n", errstr);
		 exit(1);
		}
	}

	void asyncRead(const char *topic) {
		// asynchronous commitsa
		rd_kafka_topic_t *rkt;
		rkt = rd_kafka_topic_new(rk, topic, topic_conf);
		int32_t partition = 0;
		int64_t start_offset = 0;
		if (rd_kafka_consume_start(rkt, partition, start_offset) == -1) {
		  fprintf(stderr, "%% Failed to consume from topic %s: %s\n",
		                  topic, 
		                  rd_kafka_err2str(rd_kafka_last_error())
		         );
		}
		
		rd_kafka_resp_err_t err;
		
		bool running = true;
		while (running) {
		  /* Poll for errors, etc. */
		  rd_kafka_poll(rk, 0);
		  rd_kafka_message_t *rkmessage = rd_kafka_consume(rkt, partition, 500);
		  if (rkmessage) {
		    // handle rkmessage
		    printf("%.*s\n", (int)rkmessage->len, (char *)rkmessage->payload);
		    rd_kafka_message_destroy(rkmessage);
		  }
		}
		
		rd_kafka_consume_stop(rkt, partition);
		err = rd_kafka_consumer_close(rk);
		if (err)
		  fprintf(stderr, "%% Failed to close consumer: %s\n", rd_kafka_err2str(err));
		else
		  fprintf(stderr, "%% Consumer closed\n");
		rd_kafka_destroy(rk);
	}

	//	while (running) {
	//	 rd_kafka_message_t *rkmessage = rd_kafka_consumer_poll(rk, 500);
	//	 if (rkmessage) {
	//	   msg_process(rkmessage);
	//	   rd_kafka_message_destroy(rkmessage);
	//	
	//	   if ((++msg_count % MIN_COMMIT_COUNT) == 0)
	//	     rd_kafka_commit(rk, NULL, 0);
	//	 }
	//	}

	//	//while (running) {
	//	//  rd_kafka_message_t *rkmessage = rd_kafka_consumer_poll(rk, 1000);
	//	//  if (!rkmessage)
	//	//    continue; // timeout: no message
	//	//
	//	//  msg_process(rkmessage); // application-specific processing
	//	//  rd_kafka_message_destroy(rkmessage);
	//	//
	//	//  if ((++msg_count % MIN_COMMIT_COUNT) == 0) {
	//	//    rd_kafka_resp_err_t err = rd_kafka_commit(rk, NULL, 0);
	//	//    if (err) {
	//	//      // application-specific rollback of processed records
	//	//    }
	//	//  }
	//	//}

	//	// basic poll loop
	//	//static int shutdown = 0;
	//	//static void msg_process(rd_kafka_message_t message);
	//	//
	//	//void basic_consume_loop(rd_kafka_t *rk,
	//	//                        rd_kafka_topic_partition_list_t *topics) {
	//	//  rd_kafka_resp_err_t err;
	//	//
	//	//  if ((err = rd_kafka_subscribe(rk, topics))) {
	//	//    fprintf(stderr, "%% Failed to start consuming topics: %s\n", rd_kafka_err2str(err));
	//	//    exit(1);
	//	//  }
	//	//
	//	//  while (running) {
	//	//    rd_kafka_message_t *rkmessage = rd_kafka_consumer_poll(rk, 500);
	//	//    if (rkmessage) {
	//	//      msg_process(rkmessage);
	//	//      rd_kafka_message_destroy(rkmessage);
	//	//    }
	//	//  }
	//	//
	//	//  err = rd_kafka_consumer_close(rk);
	//	//  if (err)
	//	//    fprintf(stderr, "%% Failed to close consumer: %s\n", rd_kafka_err2str(err));
	//	//  else
	//	//    fprintf(stderr, "%% Consumer closed\n");
	//	//}

	//	// synchronous commits
	//	//void consume_loop(rd_kafka_t *rk,
	//	//                  rd_kafka_topic_partition_list_t *topics) {
	//	//  static const int MIN_COMMIT_COUNT = 1000;
	//	//
	//	//  int msg_count = 0;
	//	//  rd_kafka_resp_err_t err;
	//	//
	//	//  if ((err = rd_kafka_subscribe(rk, topics))) {
	//	//    fprintf(stderr, "%% Failed to start consuming topics: %s\n", rd_kafka_err2str(err));
	//	//    exit(1);
	//	//  }
	//	//
	//	//  while (running) {
	//	//    rd_kafka_message_t *rkmessage = rd_kafka_consumer_poll(rk, 500);
	//	//    if (rkmessage) {
	//	//      msg_process(rkmessage);
	//	//      rd_kafka_message_destroy(rkmessage);
	//	//
	//	//      if ((++msg_count % MIN_COMMIT_COUNT) == 0)
	//	//        rd_kafka_commit(rk, NULL, 0);
	//	//    }
	//	// }
	//	//
	//	//  err = rd_kafka_consumer_close(rk);
	//	//  if (err)
	//	//    fprintf(stderr, "%% Failed to close consumer: %s\n", rd_kafka_err2str(err));
	//	//  else
	//	//    fprintf(stderr, "%% Consumer closed\n");
	//	//}

	//	// delivery guarntees
	//	//void consume_loop(rd_kafka_t *rk,
	//	//                  rd_kafka_topic_partition_list_t *topics) {
	//	//  rd_kafka_resp_err_t err;
	//	//
	//	//  if ((err = rd_kafka_subscribe(rk, topics))) {
	//	//    fprintf(stderr, "%% Failed to start consuming topics: %s\n", rd_kafka_err2str(err));
	//	//    exit(1);
	//	//  }
	//	//
	//	//  while (running) {
	//	//    rd_kafka_message_t *rkmessage = rd_kafka_consumer_poll(rk, 500);
	//	//    if (rkmessage && !rd_kafka_commit_message(rk, rkmessage, 0)) {
	//	//      msg_process(rkmessage);
	//	//      rd_kafka_message_destroy(rkmessage);
	//	//    }
	//	//  }
	//	//
	//	//  err = rd_kafka_consumer_close(rk);
	//	//  if (err)
	//	//    fprintf(stderr, "%% Failed to close consumer: %s\n", rd_kafka_err2str(err));
	//	//  else
	//	//    fprintf(stderr, "%% Consumer closed\n");
	//	//}

	//	// asynchronous commits
	//	//void consume_loop(rd_kafka_t *rk,
	//	//                  rd_kafka_topic_partition_list_t *topics) {
	//	//  static const int MIN_COMMIT_COUNT = 1000;
	//	//
	//	//  int msg_count = 0;
	//	//  rd_kafka_resp_err_t err;
	//	//
	//	//  if ((err = rd_kafka_subscribe(rk, topics))) {
	//	//    fprintf(stderr, "%% Failed to start consuming topics: %s\n", rd_kafka_err2str(err));
	//	//    exit(1);
	//	//  }
	//	//
	//	//  while (running) {
	//	//    rd_kafka_message_t *rkmessage = rd_kafka_consumer_poll(rk, 500);
	//	//    if (rkmessage) {
	//	//      msg_process(rkmessage);
	//	//      rd_kafka_message_destroy(rkmessage);
	//	//
	//	//      if ((++msg_count % MIN_COMMIT_COUNT) == 0)
	//	//        rd_kafka_commit(rk, NULL, 1);
	//	//    }
	//	//  }
	//	//
	//	//  err = rd_kafka_consumer_close(rk);
	//	//  if (err)
	//	//    fprintf(stderr, "%% Failed to close consumer: %s\n", rd_kafka_err2str(err));
	//	//  else
	//	//    fprintf(stderr, "%% Consumer closed\n");
	//	//}

	//	//static void on_commit(rd_kafka_t *rk,
	//	//                      rd_kafka_resp_err_t err,
	//	//                      rd_kafka_topic_partition_list_t *offsets,
	//	//                      void *opaque) {
	//	//  if (err)
	//	//    fprintf(stderr, "%% Failed to commit offsets: %s\n", rd_kafka_err2str(err));
	//	//}
	//	//
	//	//void init_rd_kafka() {
	//	//  rd_kafka_conf_t *conf = rd_kafka_conf_new();
	//	//  rd_kafka_conf_set_offset_commit_cb(conf, on_commit);
	//	//
	//	//  // initialization omitted
	//	//}

	
};


