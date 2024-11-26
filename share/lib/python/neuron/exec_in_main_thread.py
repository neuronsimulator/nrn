import threading
import queue
import sys
import signal
import readline

# Queue for tasks to be executed in the main thread
task_queue = queue.Queue()


# Signal Handler to wake up the REPL to process tasks
def signal_handler(signum, frame):
    _run_tasks()


def submit_task_to_main_thread(task, *args, **kwargs):
    """Submit a task to be run in the main thread."""
    task_queue.put((task, args, kwargs))
    if sys.platform.startswith("win"):
        _run_tasks()  # Directly run if Windows
    else:
        # Send a signal to the main thread to ensure tasks are processed
        signal.pthread_kill(threading.main_thread().ident, signal.SIGUSR1)


def _run_tasks():
    """Process tasks in the queue to be run in the main thread."""
    while not task_queue.empty():
        try:
            task, args, kwargs = task_queue.get_nowait()
            task(*args, **kwargs)
            task_queue.task_done()
        except queue.Empty:
            break


# Pre-input hook function to process tasks before user input is taken
def pre_input_hook():
    _run_tasks()
    readline.redisplay()


readline.set_pre_input_hook(pre_input_hook)

# Catch signal for task processing
signal.signal(signal.SIGUSR1, signal_handler)


if __name__ == "__main__":

    def example_task(message):
        print(f"Task executed: {message} in {threading.current_thread().name}")

    def submit():
        # Submit an example task from another thread
        def submit_from_thread():
            print(f"enter submit_from_thread in {threading.current_thread().name}")
            submit_task_to_main_thread(example_task, "Hello from ChildThread")

        # Example thread usage
        child_thread = threading.Thread(target=submit_from_thread)
        child_thread.start()
        child_thread.join()

    submit()
