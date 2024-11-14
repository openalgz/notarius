#pragma once

namespace slx {
   /**
    * @brief A simple thread pool for managing and executing tasks. see: 'notarius::forward_to'
    */
   struct thread_pool_t final
   {
      /**
       * @brief Represents a task with a function, priority, and enqueue time.
       */
      struct Task final
      {
         std::function<void()> func; ///< The function to be executed.
         int priority; ///< The priority of the task.
         std::chrono::steady_clock::time_point enqueue_time; ///< The time when the task was enqueued.

         /**
          * @brief Constructs a Task.
          * @param f The function to be executed.
          * @param p The priority of the task.
          */
         Task(std::function<void()> f, int p)
            : func(std::move(f)), priority(p), enqueue_time(std::chrono::steady_clock::now())
         {}

         /**
          * @brief Comparison operator for priority queue.
          * @param other The other task to compare with.
          * @return True if this task has lower priority than the other task.
          */
         bool operator<(const Task& other) const
         {
            if (priority == other.priority) return enqueue_time > other.enqueue_time;
            return priority < other.priority;
         }
      };

      /**
       * @brief Constructs a thread pool.
       * @param initial_threads The initial number of threads in the pool.
       * @param max_queue_size The maximum size of the task queue.
       */
      explicit thread_pool_t(size_t initial_threads = std::thread::hardware_concurrency(), size_t max_queue_size = 1000)
         : max_queue_size_(max_queue_size)
      {
         try {
            resize(initial_threads);
         }
         catch (...) {
            stop_requested_ = true;
            throw;
         }
      }

      /**
       * @brief Destructs the thread pool and stops all threads.
       */
      ~thread_pool_t() { stop(true); }

      // Non-copyable and non-movable
      thread_pool_t(const thread_pool_t&) = delete;
      thread_pool_t& operator=(const thread_pool_t&) = delete;
      thread_pool_t(thread_pool_t&&) = delete;
      thread_pool_t& operator=(thread_pool_t&&) = delete;

     private:
      void remove_oldest_task_(std::priority_queue<Task>& tasks)
      {
         if (tasks.empty()) return;

         // Move all tasks to a temporary vector
         std::vector<Task> temp_tasks;
         while (!tasks.empty()) {
            temp_tasks.push_back(tasks.top());
            tasks.pop();
         }

         // Find the oldest task (based on enqueue_time)
         auto oldest_task = std::min_element(temp_tasks.begin(), temp_tasks.end(), [](const Task& a, const Task& b) {
            return a.enqueue_time < b.enqueue_time;
         });

         // Remove the oldest task from the temporary vector
         if (oldest_task != temp_tasks.end()) {
            temp_tasks.erase(oldest_task);
         }

         // Rebuild the priority queue from the remaining tasks
         for (const auto& task : temp_tasks) {
            tasks.push(task);
         }
      }

     public:
      /**
       * @brief Enqueues a task into the thread pool.
       * @tparam F The type of the function.
       * @tparam Args The types of the arguments.
       * @param f The function to be executed.
       * @param args The arguments to the function.
       * @param priority The priority of the task.
       * @return A future that will hold the result of the function.
       */
      template <class F, class... Args>
      auto enqueue(F&& f, Args&&... args, int priority = 0) -> std::future<std::invoke_result_t<F, Args...>>
      {
         using return_type = std::invoke_result_t<F, Args...>;
         auto task = std::make_shared<std::packaged_task<return_type()>>(
            [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable {
               return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            });
         std::future<return_type> res = task->get_future();

         {
            std::unique_lock lock(queue_mutex_);
            if (stop_requested_) {
               throw std::runtime_error("enqueue on stopped thread_pool_t");
            }
            if (tasks_.size() >= max_queue_size_) {
               assert(false && "You should increase your thread pool size!");
               remove_oldest_task_(tasks_);
               // std::priority_queue<Task> empty;
               // std::swap(tasks_, empty);
            }
            tasks_.emplace(Task(
               [task = std::move(task)]() {
                  try {
                     (*task)();
                  }
                  catch (const std::exception& e) {
                     std::cerr << "Exception in task: " << e.what() << " (Thread ID: " << std::this_thread::get_id()
                               << ")" << std::endl;
                  }
                  catch (...) {
                     std::cerr << "Unknown exception in task (Thread ID: " << std::this_thread::get_id() << ")"
                               << std::endl;
                  }
               },
               priority));
         }
         cv_.notify_one();
         return res;
      }

      /**
       * @brief Stops the thread pool and optionally waits for all tasks to complete.
       * @param wait_for_tasks If true, waits for all tasks to complete before stopping.
       */
      void stop(const bool wait_for_tasks = true)
      {
         {
            std::unique_lock lock(queue_mutex_);
            stop_requested_ = true;
         }

         cv_.notify_all();

         if (wait_for_tasks) {
            for (auto& worker : workers_) {
               if (worker.joinable()) {
                  worker.join();
               }
            }
         }

         workers_.clear();

         {
            std::unique_lock lock(queue_mutex_);
            stop_requested_ = false;
            tasks_ = std::priority_queue<Task>();
         }
      }

      /**
       * @brief Resizes the thread pool.
       * @param new_size The new number of threads in the pool.
       */
      void resize(size_t new_size)
      {
         stop(true);
         workers_.clear();
         workers_.reserve(new_size);
         for (size_t i = 0; i < new_size; ++i) {
            add_worker();
         }
      }

      /**
       * @brief Gets the number of threads in the pool.
       * @return The number of threads.
       */
      size_t size() const { return workers_.size(); }

      /**
       * @brief Gets the number of tasks in the queue.
       * @return The number of tasks.
       */
      size_t queue_size() const
      {
         std::unique_lock lock(queue_mutex_);
         return tasks_.size();
      }

      /**
       * @brief Waits for all tasks to complete.
       */
      void wait_for_tasks()
      {
         std::unique_lock lock(queue_mutex_);
         cv_.wait(lock, [this] { return tasks_.empty() && (active_threads_ == 0); });
      }

      /**
       * @brief Checks if the thread pool is stopped.
       * @return True if the thread pool is stopped, false otherwise.
       */
      bool is_stopped() const
      {
         std::unique_lock lock(queue_mutex_);
         return stop_requested_.load();
      }

     private:
      /**
       * @brief Adds a worker thread to the pool.
       */
      void add_worker()
      {
         workers_.emplace_back([this] {
            while (true) {
               std::optional<Task> task;
               {
                  std::unique_lock lock(queue_mutex_);
                  cv_.wait(lock, [this] { return !tasks_.empty() || stop_requested_.load(); });
                  if (stop_requested_.load() && tasks_.empty()) {
                     return;
                  }
                  if (!tasks_.empty()) {
                     task = std::move(const_cast<Task&>(tasks_.top()));
                     tasks_.pop();
                  }
               }
               if (task) {
                  ++active_threads_;
                  task->func();
                  --active_threads_;
               }
            }
         });
      }

      // TODO: std::latch

      std::vector<std::thread> workers_; ///< The worker threads.
      std::priority_queue<Task> tasks_; ///< The task queue.
      mutable std::mutex queue_mutex_; ///< Mutex for synchronizing access to the task queue.
      std::condition_variable cv_; ///< Condition variable for notifying worker threads.
      std::atomic_bool stop_requested_{false}; ///< Flag indicating if the thread pool is stopped.
      std::atomic<size_t> active_threads_{0}; ///< The number of active threads.
      size_t max_queue_size_; ///< The maximum size of the task queue.
   };   
}