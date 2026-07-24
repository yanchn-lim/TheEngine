(() => {
  const systems = [
    {
      name: 'Engine',
      namespaces: [{
        name: 'global',
        items: [
          { kind: 'page', page: 'overview', title: 'Overview', href: 'index.html', terms: 'architecture scope status reading path' },
          { kind: 'page', page: 'getting-started', title: 'Build and run', href: 'getting-started.html', terms: 'requirements Visual Studio Vulkan SDK backend lifecycle update loop shutdown' },
          { kind: 'class', title: 'Engine', href: 'getting-started.html#initialization', terms: 'startup initialization shutdown core' }
        ]
      }]
    },
    {
      name: 'Rendering',
      namespaces: [{
        name: 'Rendering',
        items: [
          { kind: 'page', page: 'rendering', title: 'Rendering overview', href: 'rendering.html', terms: 'persistent transient draw mesh sprite frame sorting layers' },
          { kind: 'class', page: 'render-world', title: 'RenderWorld', href: 'rendering-renderworld.html', terms: 'render instances persistent one-frame submission' },
          { kind: 'class', page: 'renderer', title: 'Renderer', href: 'rendering-renderer.html', terms: 'collect sort execute render items' },
          { kind: 'class', title: 'RenderResourceManager', href: 'assets.html#resource-manager', terms: 'resolve cache GPU resources invalidation' }
        ]
      }]
    },
    {
      name: 'Graphics',
      namespaces: [{
        name: 'Graphics',
        items: [
          { kind: 'page', page: 'graphics-api', title: 'Graphics overview', href: 'graphics-api.html', terms: 'handles resources descriptors pipeline frame contract limits' },
          { kind: 'class', page: 'graphics-device', title: 'IGraphicsDevice', href: 'graphics-igraphicsdevice.html', terms: 'resource ownership frame submission presentation' },
          { kind: 'class', page: 'graphics-command-list', title: 'IGraphicsCommandList', href: 'graphics-igraphicscommandlist.html', terms: 'commands render pass draw state' },
          { kind: 'page', page: 'backends', title: 'Backends', href: 'backends.html', terms: 'OpenGL Vulkan swapchain ImGui resize device lost' },
          { kind: 'class', title: 'OpenGLGraphicsDevice', href: 'backends.html#opengl', terms: 'OpenGL backend' },
          { kind: 'class', title: 'VulkanGraphicsDevice', href: 'backends.html#vulkan', terms: 'Vulkan backend' }
        ]
      }]
    },
    {
      name: 'Assets',
      namespaces: [{
        name: 'Assets',
        items: [
          { kind: 'page', page: 'assets', title: 'Assets overview', href: 'assets.html', terms: 'registries import OBJ texture shader mesh material invalidation' },
          { kind: 'class', page: 'asset-manager', title: 'AssetManager', href: 'assets-assetmanager.html', terms: 'asset registries handles records import' }
        ]
      }]
    },
    {
      name: 'ECS',
      namespaces: [{
        name: 'ECS',
        items: [
          { kind: 'page', page: 'ecs', title: 'ECS overview', href: 'ecs.html', terms: 'entity component sparse set pool view query iteration' },
          { kind: 'class', page: 'ecs-world', title: 'World', href: 'ecs-world.html', terms: 'entities component access views foreach queries' },
          { kind: 'struct', title: 'Entity', href: 'ecs.html#entities', terms: 'entity id generation alive stale handle' },
          { kind: 'struct', title: 'ComponentView', href: 'ecs-world.html#views', terms: 'span dense components entities view' }
        ]
      }]
    },
    {
      name: 'Diagnostics',
      namespaces: [
        {
          name: 'global',
          items: [
            { kind: 'page', page: 'diagnostics', title: 'Diagnostics and tests', href: 'diagnostics.html', terms: 'timing memory logging F5 F6 tests troubleshooting' },
            { kind: 'class', title: 'Profiler', href: 'diagnostics.html#profiler', terms: 'frame scope timing samples' },
            { kind: 'class', title: 'DebugConsole', href: 'diagnostics.html#logging', terms: 'logging console messages' }
          ]
        },
        {
          name: 'Memory',
          items: [
            { kind: 'class', title: 'ResourceUsage', href: 'diagnostics.html#memory', terms: 'CPU GPU allocation memory tracking' }
          ]
        }
      ]
    },
    {
      name: 'Repository',
      namespaces: [{
        name: 'source',
        items: [
          { kind: 'page', page: 'source-map', title: 'Source map', href: 'source-map.html', terms: 'files folders paths glossary find code' }
        ]
      }]
    }
  ];

  const searchItems = systems.flatMap((system) => system.namespaces.flatMap((namespace) =>
    namespace.items.map((item) => ({ ...item, system: system.name, namespace: namespace.name }))
  ));

  const body = document.body;
  const currentPage = body.dataset.page || 'overview';
  const nav = document.getElementById('site-nav');
  const mobileBar = document.getElementById('mobile-bar');

  if (nav) {
    nav.innerHTML = `
      <a class="brand" href="index.html"><strong>TheEngine</strong><span>offline codebase reference</span></a>
      <div class="search-wrap">
        <label for="doc-search">search documentation</label>
        <input class="doc-search" id="doc-search" type="search" placeholder="renderer, Vulkan, assets..." autocomplete="off">
        <div class="search-results" id="search-results" aria-live="polite"></div>
      </div>
      ${systems.map((system) => `
        <section class="nav-system">
          <p class="nav-heading">${system.name}</p>
          ${system.namespaces.map((namespace) => `
            <p class="nav-namespace">namespace <code>${namespace.name}</code></p>
            <ul class="nav-list">
              ${namespace.items.map((item) => `
                <li><a href="${item.href}" class="${item.kind === 'page' ? 'page-link' : 'type-link'}${item.page === currentPage ? ' active' : ''}"${item.page === currentPage ? ' aria-current="page"' : ''}>${item.kind === 'page' ? item.title : `<code>${item.title}</code>`}</a></li>
              `).join('')}
            </ul>
          `).join('')}
        </section>
      `).join('')}
    `;

    const search = document.getElementById('doc-search');
    const results = document.getElementById('search-results');

    search.addEventListener('input', () => {
      const query = search.value.trim().toLowerCase();
      if (!query) {
        results.classList.remove('open');
        results.innerHTML = '';
        return;
      }

      const matches = searchItems.filter((item) => `${item.system} ${item.namespace} ${item.title} ${item.terms}`.toLowerCase().includes(query));
      results.innerHTML = matches.length
        ? matches.map((item) => `<a href="${item.href}"><strong>${item.title}</strong><span>${item.system} · ${item.namespace} · ${item.kind}</span></a>`).join('')
        : '<p class="search-empty">No matching page.</p>';
      results.classList.add('open');
    });
  }

  if (mobileBar) {
    mobileBar.innerHTML = '<button class="menu-button" id="menu-button" type="button" aria-controls="site-nav" aria-expanded="false">menu</button><span>TheEngine reference</span>';
    const menuButton = document.getElementById('menu-button');
    menuButton.addEventListener('click', () => {
      const open = nav.classList.toggle('open');
      menuButton.setAttribute('aria-expanded', String(open));
    });
  }

  const pageContent = document.querySelector('.page-content');
  const pageSections = pageContent
    ? [...pageContent.children].filter((element) => element.tagName === 'SECTION' && element.id)
    : [];

  if (pageSections.length) {
    const sectionOverview = document.createElement('nav');
    sectionOverview.className = 'section-overview';
    sectionOverview.setAttribute('aria-label', 'Page sections');
    sectionOverview.innerHTML = `
      <strong>At a glance</strong>
      <div>
        ${pageSections.map((section, index) => {
          const heading = section.querySelector('h2');
          return `<a href="#${section.id}"><span>${String(index + 1).padStart(2, '0')}</span>${heading ? heading.textContent : section.id}</a>`;
        }).join('')}
      </div>
    `;

    const sectionGrid = document.createElement('div');
    sectionGrid.className = 'doc-sections';
    pageContent.insertBefore(sectionOverview, pageSections[0]);
    sectionOverview.after(sectionGrid);

    pageSections.forEach((section) => {
      const hasDenseContent = section.querySelector('.api-list, .path-list, .cards, .system-list, .flow, pre, .steps');
      section.classList.add('doc-section');
      if (!hasDenseContent && section.textContent.trim().length < 1000) {
        section.classList.add('compact-section');
      }
      sectionGrid.append(section);
    });

    if (window.location.hash) {
      const scrollToHash = () => window.requestAnimationFrame(() => {
        window.requestAnimationFrame(() => {
          document.getElementById(window.location.hash.slice(1))?.scrollIntoView();
        });
      });

      if (document.readyState === 'complete') scrollToHash();
      else window.addEventListener('load', scrollToHash, { once: true });
    }
  }

  const tocLinks = [...document.querySelectorAll('.toc a')];
  const headings = tocLinks
    .map((link) => document.querySelector(link.hash))
    .filter(Boolean);

  if (headings.length) {
    const observer = new IntersectionObserver((entries) => {
      const first = entries
        .filter((entry) => entry.isIntersecting)
        .sort((left, right) => left.boundingClientRect.top - right.boundingClientRect.top)[0];
      if (!first) return;
      tocLinks.forEach((link) => link.classList.toggle('active', link.hash === `#${first.target.id}`));
    }, { rootMargin: '-12% 0px -75% 0px' });
    headings.forEach((heading) => observer.observe(heading));
  }
})();
