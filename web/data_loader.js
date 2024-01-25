//
// Data Loader
//

// Are we running on GitHub Pages?
export function isRunningOnGitHubPages() {
  return window.location.hostname.includes("github.io");
}

async function getDataWithProgress(url) {
  const response = await fetch(url);

  const reader = response.body.getReader();
  const contentLength = +response.headers.get("Content-Length");
  const loadingBar = document.getElementById("loading_bar_data");

  if (contentLength && contentLength > 250) {
    loadingBar.style.display = "block";
  } else {
    loadingBar.style.display = "none";
  }

  let receivedLength = 0;
  let chunks = [];

  while (true) {
    const { done, value } = await reader.read();

    if (done) {
      break;
    }

    chunks.push(value);
    receivedLength += value.length;

    const percent = Math.round((receivedLength / contentLength) * 100);

    if (contentLength && contentLength > 250)
      loadingBar.style.width = percent + "%";
  }

  let chunksAll = new Uint8Array(receivedLength);
  let position = 0;
  for (let chunk of chunks) {
    chunksAll.set(chunk, position);
    position += chunk.length;
  }

  let result = new TextDecoder("utf-8").decode(chunksAll);

  if (contentLength && contentLength > 250) loadingBar.style.display = "none";

  return JSON.parse(result);
}

// Simple function to pull down all data from the server.
async function loadServerData() {
  let detectorGeometry = getDataWithProgress("geometry");
  let hits = getDataWithProgress("hits");
  let mcHits = getDataWithProgress("mcHits");
  let markers = getDataWithProgress("markers");
  let particles = getDataWithProgress("particles");

  // Wait for all the data to be loaded.
  [hits, mcHits, markers, particles, detectorGeometry] = await Promise.all([
    hits,
    mcHits,
    markers,
    particles,
    detectorGeometry,
  ]);

  return {
    hits: hits,
    mcHits: mcHits,
    markers: markers,
    particles: particles,
    detectorGeometry: detectorGeometry,
  };
}

// Load data from a GitHub Gist URL.
async function loadExternalData(url) {
  // Now, request the data from the supplied GitHub Gist URL.
  // First, just check tha the URL is valid and is a raw URL, not a
  // link to the GitHub page.
  if (!url.includes("gist.githubusercontent.com")) {
    console.error("Invalid URL for GitHub Gist");
    return;
  }

  return await getDataWithProgress(url);
}

// Top-level function to load data from the server or from a GitHub Gist URL.
export async function getData() {
  if (isRunningOnGitHubPages()) {
    return loadExternalData(
      "https://gist.githubusercontent.com/CrossR/f0ab94b5d945d58742586a16eb10bcf4/raw/4227132a6846cfe9729e3a28fa1619a7e2f5a8b1/testEvent.json"
    );
  } else {
    return loadServerData();
  }
}
