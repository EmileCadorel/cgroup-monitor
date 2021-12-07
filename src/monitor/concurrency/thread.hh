#pragma once
#include <pthread.h>

namespace monitor {

    namespace concurrency {

	typedef pthread_t thread;
	
	namespace internal {
	    void* thread_fn_main (void * inner);
	    void* thread_dg_main (void * inner);

	    class fake {};

	    
	    class dg_thread_launcher {
	    public:
		thread content;
		fake* closure;
		void (fake::*func) (thread);

		dg_thread_launcher (fake* closure, void (fake::*func) (thread));		
	    };

	    class fn_thread_launcher {
	    public:
		thread content;
		void (*func) (thread);

		fn_thread_launcher (void (*func) (thread));		
	    };
	    
	    
	}

	/**
	 * Spawn a new thread that will run a function
	 * @params: 
	 *  - func: the main function of the thread
	 */
	thread spawn (void (*func) (thread));

	/**
	 * Spawn a new thread that will run a method
	 * @params: 
	 *  - func: the main method of the thread
	 */
	template <class X>
	thread spawn (X * x, void (X::*func)(thread)) {
	    auto th = new internal::dg_thread_launcher ((internal::fake*) x, (void (internal::fake::*)(thread)) func);
	    pthread_create (&th-> content, nullptr, &internal::thread_dg_main, th);
	    return th-> content;
	}

	/**
	 * Wait the end of the execution of the thread
	 * @params: 
	 *  - th: the thread to wait
	 */
	void join (thread th);

	/**
	 * Kill a running thread
	 * @params: 
	 *  - th: the thread to kill
	 */
	void kill (thread th);
	
    }

}
