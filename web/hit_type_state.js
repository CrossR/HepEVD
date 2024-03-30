//
// Current Hit Type State
//
// A top level state object that keeps track of the current hit types.
// This is used by pretty much every other state object to determine what to show.

import { getHitTypes } from "./helpers.js";

export class HitTypeState {
  constructor(particles, hits) {
    this.types = getHitTypes(particles, hits);

    this.activeTypes = new Set();
  }

  /**
   * Toggles the active state of a hit type.
   *
   * @param {string} type - The hit type to toggle.
   */
  toggleHitType(type) {
    if (this.activeTypes.has(type)) {
      this.activeTypes.delete(type);
    } else {
      this.activeTypes.add(type);
    }
  }

  /**
   * Checks if the hit type is active.
   *
   * @param {Object} data - The hit data.
   * @returns {boolean} - True if the hit type is active, false otherwise.
   */
  checkHitType(data) {
    return (
      this.activeTypes.size === 0 || this.activeTypes.has(data.position.hitType)
    );
  }
}
