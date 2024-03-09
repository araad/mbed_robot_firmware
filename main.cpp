#include "mbed-trace/mbed_trace.h"
#include "mbed.h"
#include "megapi-proxy.h"
#include "transports/transports.h"
#include <megapi_proxy_msgs/msg/motors_speed.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>

#define TRACE_GROUP "MAIN"

#define RCCHECK(fn)                                                                      \
	{                                                                                    \
		rcl_ret_t temp_rc = fn;                                                          \
		if ((temp_rc != RCL_RET_OK))                                                     \
		{                                                                                \
			printf("Failed status on line %d: %d. Aborting.\n", __LINE__, (int)temp_rc); \
			while (1)                                                                    \
			{                                                                            \
			};                                                                           \
		}                                                                                \
	}

static Mutex stats_mutex;
static Mutex trace_mutex;
static void trace_mutex_wait()
{
	trace_mutex.lock();
}
static void trace_mutex_release()
{
	trace_mutex.unlock();
}

MegaPiProxy mega_pi(PA_0, PA_1);

EventQueue events_queue(12 * EVENTS_EVENT_SIZE);
Thread micro_ros_thread(osPriorityNormal, 8 * OS_STACK_SIZE);

rclc_support_t support;
rcl_node_t megapi_node;
rclc_executor_t executor;

rcl_subscription_t motors_subscriber;
megapi_proxy_msgs__msg__MotorsSpeed motors_sub_msg;

void encoder_motors_run(int16_t left_speed, int16_t right_speed)
{
	mega_pi.encoder_motors_run(left_speed, right_speed);
}

void motors_subscriber_callback(const void *msgin)
{
	const megapi_proxy_msgs__msg__MotorsSpeed *msg = (const megapi_proxy_msgs__msg__MotorsSpeed *)msgin;
	events_queue.call(encoder_motors_run, msg->left_speed, msg->right_speed);
}

void micro_ros_task()
{
	tr_info("micro_ros_task - begin (Thread Id: %d)", ThisThread::get_id());

	auto init_ret = init_transport();

	if (!init_ret)
	{
		tr_error("init_transport failed, terminating micro_ros_task");
		return;
	}

	rcl_allocator_t allocator = rcl_get_default_allocator();
	RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
	RCCHECK(rclc_node_init_default(&megapi_node, "megapi_node", "", &support));
	RCCHECK(rclc_subscription_init_default(&motors_subscriber, &megapi_node, ROSIDL_GET_MSG_TYPE_SUPPORT(megapi_proxy_msgs, msg, MotorsSpeed), MBED_CONF_APP_MOTORS_ROS_TOPIC_PATH))
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_subscription(&executor, &motors_subscriber, &motors_sub_msg, &motors_subscriber_callback, ON_NEW_DATA));

	// Runs forever until an error is encountered
	RCCHECK(rclc_executor_spin(&executor));

	// free resources
	RCCHECK(rcl_subscription_fini(&motors_subscriber, &megapi_node));
	RCCHECK(rcl_node_fini(&megapi_node));
}

int main()
{
	mbed_trace_mutex_wait_function_set(trace_mutex_wait);
	mbed_trace_mutex_release_function_set(trace_mutex_release);
	mbed_trace_init();

	tr_info("Starting main (Thread Id: %d)", ThisThread::get_id());

	micro_ros_thread.start(callback(micro_ros_task));

	events_queue.dispatch_forever();
}
