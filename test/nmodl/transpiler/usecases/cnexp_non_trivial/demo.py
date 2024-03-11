import numpy as np
import matplotlib.pyplot as plt


t_end = 4.0

n = 10000
x_approx = np.empty(n)
x_approx[0] = 0.1

x_exact = np.empty(n)
x_exact[0] = 0.1

dt = t_end / n
t = np.linspace(0.0, t_end, n)

for i in range(1, n):
    x_approx[i] = 1.4142135623730951 * np.sqrt(dt + 0.5 * x_approx[i - 1] ** 2.0)
    x_exact[i] = x_exact[i - 1] + dt * 1 / x_exact[i - 1]

plt.plot(t, x_approx)
plt.plot(t, x_exact)

plt.show()
