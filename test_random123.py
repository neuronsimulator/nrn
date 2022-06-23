from neuron import h
import matplotlib.pyplot as plt


def create_plot(rand, type):
    values = []
    for i in range(100000):
        values.append(rand.repick_random123())
    # plt.plot(values)
    plt.hist(values, bins=1000)
    # plt.show()
    plt.savefig("hist_{}.pdf".format(type), format="pdf")
    plt.close()


rand = h.Random()
print("Test default rand")
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "random")

print("Setup rand for Binomial distribution")
print(rand.binomial_random123(20, 0.5))
# print(rand.repick_random123())
# print(rand.binomial_random123(20, 0.5))
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "binomial")

print("Setup rand for Normal distribution")
print(rand.normal_random123(5, 2))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "normal")

print("Setup rand for LogNormal distribution")
print(rand.lognormal_random123(5, 2))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "lognormal")

print("Setup rand for Poisson distribution")
print(rand.poisson_random123(5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "poisson")

print("Setup rand for Uniform distribution")
print(rand.uniform_random123(1, 5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "uniform")

print("Setup rand for DiscUnif distribution")
print(rand.discunif_random123(1, 5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "discunif")

print("Setup rand for Weibull distribution")
print(rand.weibull_random123(1, 5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "weibull")

print("Setup rand for Erlang distribution")
print(rand.erlang_random123(1, 5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "erlang")

print("Setup rand for NegExp distribution")
print(rand.negexp_random123(1.5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "negexp")

print("Setup rand for Geom distribution")
print(rand.geometric_random123(0.5))
# print(rand.repick_random123())
# for i in range(10):
#     print(rand.repick_random123())
create_plot(rand, "geom")

quit()
