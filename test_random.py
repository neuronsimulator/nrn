from neuron import h
import matplotlib.pyplot as plt

def create_plot(rand, type):
    values = []
    for i in range(100000):
        values.append(rand.repick())
    # plt.plot(values)
    plt.hist(values, bins=1000)
    # plt.show()
    plt.savefig("hist_{}.pdf".format(type), format="pdf")
    plt.close()

rand = h.Random()
print("Test default rand")
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "random")

print("Setup rand for Binomial distribution")
print(rand.binomial(20, 0.5))
# print(rand.repick())
# print(rand.binomial(20, 0.5))
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "binomial")

print("Setup rand for Normal distribution")
print(rand.normal(5, 2))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "normal")

print("Setup rand for LogNormal distribution")
print(rand.lognormal(5, 2))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "lognormal")

print("Setup rand for Poisson distribution")
print(rand.poisson(5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "poisson")

print("Setup rand for Uniform distribution")
print(rand.uniform(1, 5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "uniform")

print("Setup rand for DiscUnif distribution")
print(rand.discunif(1, 5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "discunif")

print("Setup rand for Weibull distribution")
print(rand.weibull(1, 5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "weibull")

print("Setup rand for Erlang distribution")
print(rand.erlang(1, 5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "erlang")

print("Setup rand for Erlang distribution")
print(rand.negexp(1.5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "negexp")

print("Setup rand for Geom distribution")
print(rand.geometric(0.5))
# print(rand.repick())
# for i in range(10):
#     print(rand.repick())
create_plot(rand, "geom")

quit()