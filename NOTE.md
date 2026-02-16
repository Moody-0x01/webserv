## Comparison Table

-------------------------------------------------------------------------------------------------------------------
| Feature         | select            | poll              | epoll                   | kqueue                      |
|-----------------|-------------------|-------------------|-------------------------|-----------------------------|
| **Platform**    | POSIX             | POSIX             | Linux                   | BSD/macOS                   |
| **Max FDs**     | 1024              | Unlimited         | Unlimited               | Unlimited                   |
| **Performance** | O(n)              | O(n)              | O(1)                    | O(1)                        |
| **State**       | Rebuild each call | Rebuild each call | Kernel keeps state      | Kernel keeps state          |
| **Events**      | Read/Write/Error  | Read/Write/Error  | Read/Write/Error + more | Read/Write/Timer/Signal/etc |
+-----------------------------------------------------------------------------------------------------------------+

---

## Example Flow: Web Server
```
1. socket()     → Create server socket (fd 3)
                  ┌──────────┐
                  │ Socket 3 │
                  └──────────┘

2. bind()       → Bind to 0.0.0.0:8080
                  ┌──────────┐
                  │ Socket 3 │
                  │ :8080    │
                  └──────────┘

3. listen()     → Mark as listening
                  ┌──────────┐
                  │ Socket 3 │
                  │ LISTENING│
                  └──────────┘

4. epoll_create() → Create epoll (fd 10)
                  ┌──────────┐
                  │ Epoll 10 │
                  └──────────┘

5. epoll_ctl(ADD, fd 3) → Monitor server socket
                  ┌──────────┐
                  │ Epoll 10 │
                  │ Watch: 3 │
                  └──────────┘

6. epoll_wait() → Block until event
                  ┌──────────┐
                  │ Epoll 10 │
                  │ Ready: 3 │ ← New connection!
                  └──────────┘

7. accept()     → Accept connection (fd 4)
                  ┌──────────┐  ┌──────────┐
                  │ Socket 3 │  │ Socket 4 │
                  │ LISTENING│  │ Client   │
                  └──────────┘  └──────────┘

8. epoll_ctl(ADD, fd 4) → Monitor client socket
                  ┌──────────┐
                  │ Epoll 10 │
                  │ Watch:   │
                  │  • 3     │
                  │  • 4     │
                  └──────────┘

9. epoll_wait() → Block until event
                  ┌──────────┐
                  │ Epoll 10 │
                  │ Ready: 4 │ ← Client sent data!
                  └──────────┘

10. recv()      → Read client data
11. send()      → Send response
12. close()     → Close client socket
