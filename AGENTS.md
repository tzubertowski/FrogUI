# FrogUI agent instructions

## Canonical checkout and branches

- Canonical source checkout: `/home/tomaszz/sf3000-work/FrogUI`.
- For the FrogUI core used by TreeFrogUI on the R36SX/SF3000 family, integrate
  into `r36sx`. `master` is not that device integration branch.
- Work on the canonical checkout. Do not make duplicate clones or worktrees,
  including under `/tmp`; do not edit a detached submodule copy as the source
  of truth.
- Use at most one feature branch at a time. Rebase it onto the current
  integration branch while preserving tested behavior. Ask for explicit user
  approval before merging into `r36sx`.
- Keep existing system aliases, friendly labels, theme artwork fallbacks,
  translations, and the shared pill animation when integrating UI work. A
  feature branch based on an older snapshot must not replace the current
  integration source wholesale.
- After a FrogUI change is integrated, update the `frogui` gitlink in
  `/home/tomaszz/sf3000-work/sf3000_treefrogui` on TreeFrogUI `main` to that
  exact commit. Do not pin a stale feature branch snapshot.

## Preserve local work

- Inspect status before switching branches or rebasing. Preserve modified,
  untracked, and generated files; never clean or reset them to make a checkout
  tidy. Record any stash used so it can be recovered.
