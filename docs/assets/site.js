// Torqwave's site: small conveniences. Every page reads and plays without this.
(function () {
  // One clip at a time: starting a player pauses the others, and its card is marked while it plays.
  document.addEventListener("play", function (e) {
    if (! (e.target instanceof HTMLAudioElement)) return;
    for (const other of document.querySelectorAll("audio")) if (other !== e.target) other.pause();
    const card = e.target.closest(".clip");
    if (card) card.classList.add("playing");
  }, true);
  for (const type of ["pause", "ended"])
    document.addEventListener(type, function (e) {
      const card = e.target instanceof HTMLAudioElement ? e.target.closest(".clip") : null;
      if (card) card.classList.remove("playing");
    }, true);

  // Links in the Markdown pages that leave the site's folder (the README, the changelog, the source)
  // go to the same file on GitHub.
  const root = new URL(window.torqwaveRoot || "./", location.href);
  const repo = "https://github.com/BobClawblaw/torqwave/blob/main/";
  for (const a of document.querySelectorAll(".doc a[href]")) {
    const to = new URL(a.getAttribute("href"), location.href);
    if (to.origin !== location.origin || to.pathname.startsWith(root.pathname)) continue;
    const above = new URL("../", root).pathname;
    if (to.pathname.startsWith(above)) a.href = repo + to.pathname.slice(above.length) + to.hash;
  }

  // The guide: the page being read is marked in the list, and the pages before and after are linked below it.
  const side = document.querySelector(".side");
  if (side) {
    const links = Array.from(side.querySelectorAll("ol a"));
    const here = location.pathname.split("/").pop();
    const at = links.findIndex(function (a) { return a.getAttribute("href").split("/").pop() === here; });
    const shortcuts = side.querySelector("p a");
    if (at >= 0) links[at].classList.add("here");
    else if (shortcuts && here === "SHORTCUTS.html") shortcuts.classList.add("here");
    const pager = document.getElementById("pager");
    if (pager && at >= 0) {
      const link = function (a, cls, label) {
        const l = document.createElement("a");
        l.href = a.href; l.className = cls; l.textContent = label;
        pager.append(l);
      };
      if (at > 0) link(links[at - 1], "prev", "← " + links[at - 1].textContent);
      if (at < links.length - 1) link(links[at + 1], "next", links[at + 1].textContent + " →");
    }
    if (window.matchMedia("(max-width: 860px)").matches) side.querySelector("details").removeAttribute("open");
  }
})();
