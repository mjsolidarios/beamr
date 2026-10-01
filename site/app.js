// Progressive enhancement only. Navigation, FAQs, and downloads also work without JS.
const root = document.documentElement;
const toggle = document.querySelector('.theme-toggle');
const systemTheme = matchMedia('(prefers-color-scheme: dark)');

function isDark() {
  return root.dataset.theme ? root.dataset.theme === 'dark' : systemTheme.matches;
}

function updateThemeControl() {
  toggle.setAttribute('aria-label', `Switch to ${isDark() ? 'light' : 'dark'} theme`);
  document.querySelector('meta[name="theme-color"]').content = isDark() ? '#070d11' : '#f3f7f9';
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

// Scroll motion. Without GSAP the page is complete as it is, so bail quietly.
if (window.gsap && window.ScrollTrigger) {
  gsap.registerPlugin(ScrollTrigger);
  root.classList.add('motion-ready');
  initScrollState();
  initMotion();
} else {
  root.classList.remove('motion');
}

// State that follows the scroll position but doesn't move anything, so it runs
// even when reduced motion is on.
function initScrollState() {
  const nav = document.querySelector('.nav');
  const float = () => nav.classList.toggle('is-floating', scrollY > 24);
  ScrollTrigger.create({ onUpdate: float, onRefresh: float });

  // The setup rail runs from the first number to the last, however tall the steps get.
  const steps = document.querySelector('.steps');
  const rail = steps.querySelector('.rail');
  ScrollTrigger.addEventListener('refreshInit', () => {
    const dots = steps.querySelectorAll('.step-dot');
    const top = steps.getBoundingClientRect().top;
    const center = dot => { const r = dot.getBoundingClientRect(); return r.top + r.height / 2 - top; };
    rail.style.top = `${center(dots[0])}px`;
    rail.style.bottom = `${steps.offsetHeight - center(dots[dots.length - 1])}px`;
  });

  // The device stage shows whichever feature sits across the middle of the viewport.
  const stack = document.querySelector('.stack');
  const stage = stack.querySelector('.stage');
  const features = [...stack.querySelectorAll('.feature')];
  stack.classList.add('is-synced');
  for (const feature of features) {
    ScrollTrigger.create({
      trigger: feature,
      start: 'top 60%',
      end: 'bottom 60%',
      onToggle: ({ isActive }) => {
        if (!isActive) return;
        features.forEach(f => f.classList.toggle('is-active', f === feature));
        stage.dataset.state = feature.dataset.feature;
      },
    });
  }
}

function initMotion() {
  const mm = gsap.matchMedia();

  mm.add('(prefers-reduced-motion: reduce)', () => {
    document.querySelectorAll('.route-target').forEach(node => node.classList.add('is-lit'));
  });

  mm.add('(prefers-reduced-motion: no-preference)', () => {
    gsap.defaults({ ease: 'expo.out' });

    // Hero entrance: the headline rises out of its lines, then the rest settles in.
    const intro = gsap.timeline({ delay: 0.1 });
    intro
      .fromTo('h1 [data-intro]', { yPercent: 110, autoAlpha: 0 }, { yPercent: 0, autoAlpha: 1, duration: 1.2, stagger: 0.12 })
      .fromTo('.hero-side', { y: 24, autoAlpha: 0 }, { y: 0, autoAlpha: 1, duration: 1 }, 0.35)
      .fromTo('.hero-stage', { y: 48, autoAlpha: 0 }, { y: 0, autoAlpha: 1, duration: 1.3 }, 0.45);

    // "More room": the picture widens to the full window as you scroll toward it.
    const stage = document.querySelector('.hero-stage');
    const frame = stage.querySelector('.hero-frame');
    const gutter = () => document.querySelector('.hero-head').getBoundingClientRect().left;
    gsap.timeline({
      scrollTrigger: {
        trigger: stage,
        start: 0,
        end: () => `top ${document.querySelector('.nav__inner').getBoundingClientRect().bottom + 12}px`,
        scrub: 0.6,
        invalidateOnRefresh: true,
      },
    })
      .fromTo(frame,
        { clipPath: () => `inset(0px ${gutter()}px 0px ${gutter()}px round 22px)` },
        { clipPath: 'inset(0px 0px 0px 0px round 0px)', ease: 'none' })
      .fromTo(frame.querySelector('img'), { scale: 1.14 }, { scale: 1, ease: 'none' }, 0);

    // The beam: one line out of Android, one into each desktop.
    const targets = gsap.utils.toArray('.route-target');
    gsap.timeline({
      scrollTrigger: {
        trigger: '.route-map',
        start: 'top 80%',
        end: 'bottom 45%',
        scrub: 0.5,
        onUpdate: ({ progress }) => targets.forEach((node, i) => node.classList.toggle('is-lit', progress > 0.55 + i * 0.12)),
      },
    })
      .fromTo('.route-beam', { strokeDashoffset: 1 }, { strokeDashoffset: 0, ease: 'none', stagger: 0.18 })
      .fromTo(targets, { x: -16, autoAlpha: 0.35 }, { x: 0, autoAlpha: 1, ease: 'power2.out', stagger: 0.18 }, 0.25);

    // Setup: a rail fills as you read down the steps, lighting each number.
    const steps = document.querySelector('.steps');
    steps.classList.add('is-live');
    gsap.fromTo('.rail-fill', { scaleY: 0 }, {
      scaleY: 1, ease: 'none',
      scrollTrigger: { trigger: steps, start: 'top 62%', end: 'bottom 62%', scrub: 0.4 },
    });
    steps.querySelectorAll('.step').forEach(step => {
      ScrollTrigger.create({
        trigger: step,
        start: 'top 62%',
        onEnter: () => step.classList.add('is-reached'),
        onLeaveBack: () => step.classList.remove('is-reached'),
      });
    });

    // A few blocks rise in once, the first time they come into view.
    gsap.set('[data-reveal]', { y: 28, autoAlpha: 0 });
    ScrollTrigger.batch('[data-reveal]', {
      start: 'top 88%',
      once: true,
      onEnter: batch => gsap.to(batch, { y: 0, autoAlpha: 1, duration: 1, stagger: 0.08, overwrite: true }),
    });

    // The closing line lights up word by word.
    const line = document.querySelector('.footer-line');
    const text = line.textContent;
    line.innerHTML = `<span class="sr-only">${text}</span>`
      + text.split(' ').map(word => `<span class="w" aria-hidden="true">${word}</span>`).join(' ');
    gsap.fromTo(line.querySelectorAll('.w'), { opacity: 0.16 }, {
      opacity: 1, ease: 'none', stagger: 0.1,
      scrollTrigger: { trigger: line, start: 'top 88%', end: 'max', scrub: 0.4 },
    });

    return () => {
      steps.classList.remove('is-live');
      line.textContent = text;
    };
  });
}
