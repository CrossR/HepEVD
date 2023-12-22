//
// Current Hit Type State
//

import { getHitTypes } from "./helpers.js";

export class HitTypeState {
  constructor(particles, hits) {
    this.types = getHitTypes(particles, hits);
    this.activeTypes = new Set();
  }

  // Property mutators
  toggleHitType(type) {
    if (this.activeTypes.has(type)) {
      this.activeTypes.delete(type);
    } else {
      this.activeTypes.add(type);
    }
  }

  checkHitType(data) {
    return this.activeTypes.size === 0 || this.activeTypes.has(data.position.hitType);
  }
}
