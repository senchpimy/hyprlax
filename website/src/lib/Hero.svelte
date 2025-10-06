<script>
  import { onMount } from 'svelte';
  export let mouseX = 0.5;
  export let mouseY = 0.5;
  // Import the demo GIF via Vite so it works in dev and build, and respects base path
  import demoGif from '../../../assets/hyprlax-demo.gif?url';

  const fetchRedditScore = async () => {
    try {
      const response = await fetch('https://www.reddit.com/comments/1nh59j7.json');
      
      if (!response.ok) {
        // Silently fail on any error including 429
        redditLoading = false;
        return;
      }
      
      const data = await response.json();
      if (data && data[0] && data[0].data && data[0].data.children && data[0].data.children[0]) {
        redditScore = data[0].data.children[0].data.score;
      }
    } catch (error) {
      // Silently fail on any error
      console.debug('Could not fetch Reddit score:', error);
    } finally {
      redditLoading = false;
    }
  };

  onMount(fetchRedditScore)

</script>

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

  <!-- CTA: Docs -->
  <!-- <div class="mt-6 flex items-center justify-center">
    <a
      href="/docs/"
      class="group inline-flex items-center gap-2 px-5 py-2 rounded-lg border border-white/10 bg-hypr-dark/60 text-hypr-blue hover:border-hypr-blue/50 hover:text-white transition-all duration-300 font-mono"
    >
      <svg class="w-5 h-5" fill="none" stroke="currentColor" stroke-width="1.8" viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
        <path stroke-linecap="round" stroke-linejoin="round" d="M12 6.253v13m0-13C10.832 5.477 9.246 5 7.5 5S4.168 5.477 3 6.253v13C4.168 18.477 5.754 18 7.5 18s3.332.477 4.5 1.253m0-13C13.168 5.477 14.754 5 16.5 5c1.747 0 3.332.477 4.5 1.253v13C19.832 18.477 18.247 18 16.5 18c-1.746 0-3.332.477-4.5 1.253"/>
      </svg>
      <span>Docs</span>
    </a>
  </div> -->
</div>

<!-- Video/Screencast -->
<div class="w-full max-w-4xl aspect-video mb-12 relative group">
  <div class="absolute inset-0 bg-gradient-to-r from-hypr-blue/20 to-hypr-pink/20 rounded-2xl blur-3xl group-hover:blur-2xl transition-all duration-500"></div>
  <div class="relative bg-hypr-dark/80 backdrop-blur-xl rounded-2xl border border-white/10 overflow-hidden shadow-2xl">
    <img 
      src={demoGif} 
      alt="Hyprlax parallax wallpaper animation demo" 
      class="w-full h-full object-cover"
      loading="lazy"
    />
  </div>
</div>

