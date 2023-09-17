//
// Data Loader
//

// Are we running on GitHub Pages?
export function isRunningOnGitHubPages() {
  return window.location.hostname.includes("github.io");
}

// Simple function to pull down all data from the server.
async function loadServerData() {
  const detectorGeometry = await fetch("geometry").then((response) =>
    response.json()
  );
  const hits = await fetch("hits").then((response) => response.json());
  const mcHits = await fetch("mcHits").then((response) => response.json());
  const markers = await fetch("markers").then((response) => response.json());
  const particles = await fetch("particles").then((response) =>
    response.json()
  );

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

  // Now, request the data from the supplied GitHub Gist URL.
  const request = new XMLHttpRequest();
  request.open("GET", url, false);
  request.send(null);

  if (request.status === 404) {
    console.error("Could not find data file");
    return;
  } else if (request.status !== 200) {
    console.error("Error loading data file");
    return;
  }

  // Now, parse the data as JSON.
  const data = JSON.parse(request.responseText);

  return data;
}

// Top-level function to load data from the server or from a GitHub Gist URL.
export async function getData() {
  if (isRunningOnGitHubPages()) {
    return loadExternalData(
      "https://gist.githubusercontent.com/CrossR/f0ab94b5d945d58742586a16eb10bcf4/raw/56df99c7d3c0e17c9bd870156b7f786fa7bf5d92/testEvent.json"
    );
  } else {
    return loadServerData();
  }
}
