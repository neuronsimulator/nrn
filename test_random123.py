from neuron import h
rand = h.Random()
print("Test default rand")
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for Binomial distribution")
print(rand.binomial_random123(20, 0.5))
print(rand.repick_random123())
print(rand.binomial_random123(20, 0.5))
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for Normal distribution")
print(rand.normal_random123(5, 2))
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for Poisson distribution")
print(rand.poisson_random123(5))
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for Uniform distribution")
print(rand.uniform_random123(1, 5))
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for DiscUnif distribution")
print(rand.discunif_random123(1, 5))
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for Weibull distribution")
print(rand.weibull_random123(1, 5))
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
print("Setup rand for Erlang distribution")
print(rand.erlang_random123(1, 5))
print(rand.repick_random123())
for i in range(10):
    print(rand.repick_random123())
quit()