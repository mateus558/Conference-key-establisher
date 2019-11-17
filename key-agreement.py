from sympy.ntheory.factor_ import totient 
import sympy
import random

psize = 10000
n = int(input("n primes: "))
m = int(input("m primes: "))
# Trent work
pn = [sympy.sieve[random.randrange(psize)] for i in range(n)]
pm = []
for i in range(m):
    p = sympy.sieve[random.randrange(psize)]
    while p in pn:
        p = sympy.sieve[random.randrange(psize)]
    pm.append(p)

print("pn: {0}\npm: {1}\n".format(pn, pm))
delta = 1
alpha = 1
beta = 1
temp_q = 1

for p in pn:
    delta *= pow(p, 3)
    alpha *= pow(p, 2)*(p-1)
    beta *= p*(p-1)*temp_q

for q in pm:
    temp_q *= q

totient_delta = totient(delta)
y = random.randrange(delta)

while sympy.igcd(y, delta) != 1:
    y = random.randrange(delta)

print("delta: {0}\n beta: {1}\n alpha: {2}\n y: {3}".format(delta, beta, alpha, y))

# Alice work
xa = random.randrange(1,delta)
xb = random.randrange(1,totient_delta)
while totient_delta % xb*beta == 0:
    xb = random.randrange(totient_delta)
gamma = alpha*pow(xa,2) + beta*xb


# Bob work
_xa = random.randrange(1,delta)
_xb = random.randrange(1,totient_delta)
while totient_delta % _xb*gamma == 0:
    _xb = random.randrange(totient_delta)
_gamma = alpha*pow(_xa,2) + beta*_xb

kab = pow(y, _gamma * xb, delta)
kba = pow(y, gamma * _xb, delta)

print("\nkab: {0}\nkba = {1}".format(kab, kba))

# Carlos entra na rede
# alice calcula gamma2 e xa2
xa2 = random.randrange(1,delta)
gamma2 = alpha*pow(xa2, 2) + beta*kab
#alice escolhe y
y = random.randrange(delta)
while sympy.igcd(y, delta) != 1:
    y = random.randrange(delta)
# carlos escolhe xc1, xc2 e calcula gamma3
xc1 = random.randrange(1,delta)
xc2 = random.randrange(1,totient_delta)
while totient_delta % xc2*gamma2 == 0:
    xc2 = random.randrange(totient_delta)
gamma3 = alpha*pow(xa2, 2) + beta*xc2

# alice e bob calculam kabc
kabc = pow(y, gamma3*kab, delta)
#carlos calcula kabc
kabc1 = pow(y, gamma2*xc2, delta)

print("\nkabc: {0}\nkabc1 = {1}".format(kabc, kabc1))
