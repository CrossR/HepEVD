//
// Marker Data State
//

import { HitTypeState } from "./hit_type_state.js";

export class MarkerDataState {
  constructor(markers) {
    this.markers = markers;
    this.activeMarkers = [];

    this.activeMarkerTypes = new Set();
  }

  /**
   * Gets the length of the activeMarkers array.
   *
   * @returns {number} The length of the activeMarkers array.
   */
  get length() {
    return this.activeMarkers.length;
  }

  /**
   * Toggles the marker type in the activeMarkerTypes set.
   *
   * @param {string} type - The marker type to toggle.
   */
  toggleMarkerType(type) {
    if (this.activeMarkerTypes.has(type)) {
      this.activeMarkerTypes.delete(type);
    } else {
      this.activeMarkerTypes.add(type);
    }
  }

  /**
   * Returns an array of markers of the specified type.
   *
   * @param {string} type - The type of markers to filter.
   * @returns {Array} - An array of markers.
   */
  getMarkersOfType(type) {
    return this.activeMarkers.filter((marker) => marker.markerType === type);
  }

  /**
   * Top level update function, to update the active markers.
   *
   * @param {Array} particles - The particles to consider for adding vertices as markers.
   * @param {HitTypeState} hitTypeState - The hit type state object used to check if a hit type is active.
   */
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
        if (!hitTypeState.checkHitType(vertex)) return;
        newMarkers.push(vertex);
      });
    });

    this.activeMarkers = newMarkers;
  }
}
