import matplotlib.pyplot as plt

# lcm[n - 1] = lcm(1, 2, ..., n)
lcm = [1, 2, 6, 12, 60, 60, 420, 840, 2520, 2520, 27720, 27720,
       360360, 360360, 360360, 720720, 12252240, 12252240,
       232792560, 232792560, 232792560, 232792560]

ns = list(range(1, len(lcm) + 1))
a = [lcm[n - 1] / n for n in ns]

bg = '#000000'
fg = '#e6e8ec'
grid = '#333333'
blue = '#4d94ff'
red = '#ff6b6b'

fig, ax = plt.subplots(figsize=(9, 5.5))
fig.patch.set_facecolor(bg)
ax.set_facecolor(bg)

ax.plot(ns, a, marker='o', color=blue)

# lcm(n) only grows from lcm(n - 1) when n is a prime power p^e, since
# that's the only time n contributes a factor not already covered by an
# earlier multiple of p. Otherwise lcm(n) == lcm(n - 1), so a(n) = lcm(n)/n
# drops relative to a(n - 1) purely because the denominator grew.
for n, v in zip(ns, a):
    if n > 1 and a[n - 2] > v:
        ax.plot(n, v, marker='o', color=red, zorder=3)

ax.set_yscale('log')
ax.set_xlabel('n (= r, base case: fully-replicated table size)', color=fg)
ax.set_ylabel('lcm(n) / n  (log scale)', color=fg)
ax.set_title('dtbl_size(r, r) = lcm(r)/r is not monotonic in r', color=fg)
ax.set_xticks(ns)
ax.grid(True, which='both', color=grid, alpha=0.6)

ax.tick_params(colors=fg)
for spine in ax.spines.values():
    spine.set_color(grid)

ax.annotate('drops in red: n is not a prime power',
            xy=(0.02, 0.96), xycoords='axes fraction',
            fontsize=9, color=red, va='top')

fig.tight_layout()
fig.savefig('dtbl_nonmonotonic.png', dpi=150, facecolor=bg)
