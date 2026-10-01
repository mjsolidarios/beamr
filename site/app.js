// Progressive enhancement only. Navigation, FAQs, and downloads also work without JS.
const root = document.documentElement;
const toggle = document.querySelector('.theme-toggle');
const systemTheme = matchMedia('(prefers-color-scheme: dark)');

function isDark() {
  return root.dataset.theme ? root.dataset.theme === 'dark' : systemTheme.matches;
}

function updateThemeControl() {
  toggle.setAttribute('aria-label', `Switch to ${isDark() ? 'light' : 'dark'} theme`);
  document.querySelector('meta[name="theme-color"]').content = isDark() ? '#0f1419' : '#f5f7f8';
}

toggle.hidden = false;
updateThemeControl();
toggle.addEventListener('click', () => {
  root.dataset.theme = isDark() ? 'light' : 'dark';
  try { localStorage.setItem('beamr-theme', root.dataset.theme); } catch { /* Private browsing can disable storage. */ }
  updateThemeControl();
});
systemTheme.addEventListener('change', updateThemeControl);

// An installation-help link should open its answer before scrolling to it.
function openLinkedAnswer() {
  const target = document.getElementById(location.hash.slice(1));
  if (target instanceof HTMLDetailsElement) target.open = true;
}
window.addEventListener('hashchange', openLinkedAnswer);
document.querySelector('a[href="#macos-help"]').addEventListener('click', () => {
  document.getElementById('macos-help').open = true;
});
openLinkedAnswer();
