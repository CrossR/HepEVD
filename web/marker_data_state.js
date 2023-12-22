//
// Marker Data State
//

export class MarkerDataState {
  constructor(markers) {
    this.markers = markers;
    this.activeMarkers = [];

    this.activeMarkerTypes = new Set();
  }

  get length() {
    return this.activeMarkers.length;
  }

  toggleMarkerType(type) {
    if (this.activeMarkerTypes.has(type)) {
      this.activeMarkerTypes.delete(type);
    } else {
      this.activeMarkerTypes.add(type);
    }
  }

  getMarkersOfType(type) {
    return this.activeMarkers.filter((marker) => marker.markerType === type);
  }

  updateActive(particles, hitTypeState) {
    if (this.activeMarkerTypes.size === 0) {
      this.activeMarkers = [];
      return;
    }

    const newMarkers = [];

    this.markers.forEach((marker) => {
      // Skip if hit type is not active
      if (!hitTypeState.checkHitType(marker)) return;

      // Skip if marker type is not active
      if (
        this.activeMarkerTypes.size > 0 &&
        !this.activeMarkerTypes.has(marker.markerType)
      ) {
        return;
      }

      newMarkers.push(marker);
    });

    // If there is particles, we can also add their vertices as markers.
    if (particles.length === 0) {
      this.activeMarkers = newMarkers;
      return;
    }

    particles.forEach((particle) => {
      particle.vertices.forEach((vertex) => {
        newMarkers.push(vertex);
      });
    });

    this.activeMarkers = newMarkers;
  }
}
