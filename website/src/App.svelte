<script>
  import { onMount } from 'svelte';
  
  let copied = false;
  let mouseX = 0.5;
  let mouseY = 0.5;
  let targetX = 0.5;
  let targetY = 0.5;
  let animationFrame;
  
  const copyCommand = () => {
    navigator.clipboard.writeText('curl -sSL https://hyprlax.com/install.sh | bash');
    copied = true;
    setTimeout(() => copied = false, 2000);
  };
  
  const handleMouseMove = (e) => {
    targetX = e.clientX / window.innerWidth;
    targetY = e.clientY / window.innerHeight;
  };
  
  const animate = () => {
    // Smooth easing with momentum - the gradient "follows" the mouse with delay
    const easing = 0.08; // Lower = more delay/smoother
    
    // Add subtle idle animation
    const time = Date.now() * 0.0001; // Very slow time factor
    const idleX = Math.sin(time) * 0.02; // Subtle horizontal drift
    const idleY = Math.cos(time * 0.7) * 0.02; // Subtle vertical drift
    
    // Combine mouse influence with idle animation
    mouseX += (targetX + idleX - mouseX) * easing;
    mouseY += (targetY + idleY - mouseY) * easing;
    
    animationFrame = requestAnimationFrame(animate);
  };
  
  onMount(() => {
    window.addEventListener('mousemove', handleMouseMove);
    animate();
    
    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      if (animationFrame) cancelAnimationFrame(animationFrame);
    };
  });
</script>

<main class="h-screen w-screen flex flex-col items-center justify-center relative overflow-hidden">
  <!-- Interactive gradient background - smooth easing movement -->
  <div 
    class="absolute inset-0"
    style="background: linear-gradient(135deg, 
           #9D00FF, 
           #050810 {40 + (mouseX - 0.5) * 10}%, 
           #FF007F {60 + (mouseY - 0.5) * 10}%, 
           #00D9FF);
           background-size: 130% 130%;
           background-position: {50 - (mouseX - 0.5) * 15}% {50 - (mouseY - 0.5) * 15}%"
  ></div>
  
  <!-- Noise texture overlay -->
  <div class="absolute inset-0 opacity-10" style="background-image: url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIzMDAiIGhlaWdodD0iMzAwIj48ZmlsdGVyIGlkPSJhIj48ZmVUdXJidWxlbmNlIHR5cGU9ImZyYWN0YWxOb2lzZSIgYmFzZUZyZXF1ZW5jeT0iLjc1IiBudW1PY3RhdmVzPSIxMCIvPjwvZmlsdGVyPjxyZWN0IHdpZHRoPSIxMDAlIiBoZWlnaHQ9IjEwMCUiIGZpbHRlcj0idXJsKCNhKSIgb3BhY2l0eT0iMC4wNSIvPjwvc3ZnPg==');"></div>
  
  <!-- Content container -->
  <div class="relative z-10 w-full max-w-6xl mx-auto px-4 sm:px-6 lg:px-8 flex flex-col items-center">
    
    <!-- Logo/Title -->
    <div class="mb-8 text-center">
      <h1 class="text-5xl sm:text-6xl md:text-7xl lg:text-8xl font-mono font-bold mb-4">
        <span class="bg-clip-text text-transparent bg-gradient-to-r from-hypr-blue to-hypr-pink">
          hyprlax
        </span>
      </h1>
      <p class="text-gray-400 text-lg sm:text-xl md:text-2xl font-mono">
        smooth parallax wallpapers for hyprland
      </p>
    </div>
    
    <!-- Video/Screencast placeholder -->
    <div class="w-full max-w-4xl aspect-video mb-12 relative group">
      <div class="absolute inset-0 bg-gradient-to-r from-hypr-blue/20 to-hypr-pink/20 rounded-2xl blur-3xl group-hover:blur-2xl transition-all duration-500"></div>
      <div class="relative bg-hypr-dark/80 backdrop-blur-xl rounded-2xl border border-white/10 overflow-hidden shadow-2xl">
        <div class="w-full h-full flex items-center justify-center">
          <!-- Placeholder for screencast -->
          <div class="text-center p-8">
            <svg class="w-16 h-16 mx-auto mb-4 text-hypr-blue animate-float" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z"></path>
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z"></path>
            </svg>
            <p class="text-gray-500 font-mono">screencast coming soon</p>
          </div>
        </div>
      </div>
    </div>
    
    <!-- Install command -->
    <div class="w-full max-w-3xl mb-8">
      <button 
        on:click={copyCommand}
        class="group w-full bg-hypr-dark/50 backdrop-blur-xl rounded-xl border border-white/10 p-4 sm:p-6 hover:border-hypr-blue/50 transition-all duration-300 relative overflow-hidden"
      >
        <div class="absolute inset-0 bg-gradient-to-r from-hypr-blue/10 to-hypr-pink/10 opacity-0 group-hover:opacity-100 transition-opacity duration-300"></div>
        <div class="relative flex items-center justify-between">
          <code class="text-sm sm:text-base md:text-lg font-mono text-gray-300">
            <span class="text-hypr-blue">$</span> curl -sSL https://hyprlax.com/install.sh <span class="text-gray-500">|</span> bash
          </code>
          <div class="ml-4 flex-shrink-0">
            {#if copied}
              <span class="text-green-400 text-sm font-mono">copied!</span>
            {:else}
              <svg class="w-5 h-5 text-gray-400 group-hover:text-hypr-blue transition-colors" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8 5H6a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2v-1M8 5a2 2 0 002 2h2a2 2 0 002-2M8 5a2 2 0 012-2h2a2 2 0 012 2m0 0h2a2 2 0 012 2v3m2 4H10m0 0l3-3m-3 3l3 3"></path>
              </svg>
            {/if}
          </div>
        </div>
      </button>
    </div>
    
    <!-- Links -->
    <div class="flex gap-6">
      <a 
        href="https://github.com/sandwichfarm/hyprlax" 
        target="_blank"
        rel="noopener noreferrer"
        class="group flex items-center gap-2 text-gray-400 hover:text-hypr-blue transition-colors font-mono"
      >
        <svg class="w-5 h-5" fill="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
          <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/>
        </svg>
        <span class="group-hover:underline">github</span>
      </a>
      
      <a 
        href="https://github.com/sandwichfarm/hyprlax/blob/master/README.md" 
        target="_blank"
        rel="noopener noreferrer"
        class="group flex items-center gap-2 text-gray-400 hover:text-hypr-pink transition-colors font-mono"
      >
        <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 6.253v13m0-13C10.832 5.477 9.246 5 7.5 5S4.168 5.477 3 6.253v13C4.168 18.477 5.754 18 7.5 18s3.332.477 4.5 1.253m0-13C13.168 5.477 14.754 5 16.5 5c1.747 0 3.332.477 4.5 1.253v13C19.832 18.477 18.247 18 16.5 18c-1.746 0-3.332.477-4.5 1.253"></path>
        </svg>
        <span class="group-hover:underline">docs</span>
      </a>
    </div>
    
  </div>
</main>

<style>
  :global(html) {
    scroll-behavior: smooth;
  }
</style>