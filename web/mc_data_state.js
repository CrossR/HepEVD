//
// MC Data State
//

export class MCDataState {
  constructor(mcHits) {
    this.mcHits = mcHits;
    this.activeMC = [];

    this.activeTypes = new Set();
  }

  // Property accessors
  get mc() {
    return this.activeMC;
  }

  // Property mutators
  toggleHitType(type) {
    if (this.activeTypes.has(type)) {
      this.activeTypes.delete(type);
    } else {
      this.activeTypes.add(type);
    }
  }

  updateActive() {
    this.activeMC = this.mcHits.filter((mcHit) => {

      // Skip if hit type is not active
      if (this.activeTypes.size > 0 && !this.activeTypes.has(mcHit.position.hitType)) {
        return false;
      }

      return true;
    });
  }
}
