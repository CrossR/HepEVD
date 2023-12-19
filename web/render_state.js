//
// Rendering State
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

import { fitSceneInCamera, setupControls } from "./camera_and_controls.js";
import { BUTTON_ID, HIT_CONFIG } from "./constants.js";
import { getHitProperties, getHitTypes, getMCColouring } from "./helpers.js";
import { drawHits, drawParticles } from "./hits.js";
import { drawPoints, drawRings } from "./markers.js";
import { drawBox } from "./rendering.js";
import {
  enableInteractionTypeToggle,
  enableMCToggle,
  isButtonActive,
  populateDropdown,
  populateMarkerToggle,
  populateTypeToggle,
  setupParticleMenu,
  toggleButton,
  updateUI,
} from "./ui.js";

/**
 * Represents the state of the rendering process, including the scene, camera,
 * detector geometry, and hit groups.
 */
export class RenderState {
  // Setup some basics, the scenes, camera, detector and hit groups.
  constructor(
    name,
    camera,
    renderer,
    particles,
    hits,
    mcHits,
    markers,
    geometry,
  ) {
    // Basic, crucial information...
    this.name = name;
    this.hitDim = name;

    // THREE.js Setup...
    this.scene = new THREE.Scene();
    this.camera = camera;
    this.controls = new OrbitControls(this.camera, renderer.domElement);

    // Setup various groups for rendering into...
    this.detGeoGroup = new THREE.Group();
    this.hitGroup = new THREE.Group();
    this.mcHitGroup = new THREE.Group();
    this.markerGroup = new THREE.Group();

    // Setup the data...
    this.updateData(particles, hits, mcHits, markers, geometry);

    // Add all the groups...
    this.scene.add(this.detGeoGroup);
    this.scene.add(this.hitGroup);
    this.scene.add(this.mcHitGroup);
    this.scene.add(this.markerGroup);

    // Finally, store a reference to the other renderer.
    // If this renderer turns on, we need to turn the other off.
    this.otherRenderer = undefined;
  }

  // Store class callbacks...
  #eventListeners = {};

  /**
   * Returns the number of hits in the current state.
   * @returns {number} The number of hits.
   */
  get hitSize() {
    if (this.particles.length > 0) return this.particles.length;
    return this.hits.length;
  }

  /**
   * Returns a boolean indicating whether the scene is currently visible.
   * @returns {boolean} Whether the scene is visible.
   */
  get visible() {
    return this.scene.visible;
  }

  /**
   * Setup event listeners. This is mainly used for hooking up rendering on change.
   */
  addEventListener(name, callback) {
    if (!this.#eventListeners[name]) this.#eventListeners[name] = [];
    this.#eventListeners[name].push(callback);
  }

  /**
   * Event trigger, which will run all the callbacks for that event.
   */
  triggerEvent(name, args) {
    this.#eventListeners[name]?.forEach((f) => f.apply(this, args));
  }

  /**
   * Updates the data used by the renderer.
   *
   * @param {Array} particles - The particles to render.
   * @param {Array} hits - The hits to render.
   * @param {Array} mcHits - The MC hits to render.
   * @param {Array} markers - The markers to render.
   * @param {Object} geometry - The detector geometry to render.
   */
  updateData(particles, hits, mcHits, markers, geometry) {
    // Data Setup, first the top level static arrays...
    this.detectorGeometry = geometry;
    this.hits = hits;
    this.mcHits = mcHits;
    this.markers = markers;

    // Filter the particles to only those that have hits in the current
    // dimension.
    this.particles = particles.flatMap((particle) => {
      const newParticle = { ...particle };
      newParticle.hits = particle.hits.filter(
        (hit) => hit.position.dim === this.hitDim,
      );
      newParticle.vertices = particle.vertices.filter(
        (vertex) => vertex.position.dim === this.hitDim,
      );

      // Ignore particles with no hits, but also
      // ensure they have no children: if they do,
      // we want to keep them as the relationship
      // between the parent and child is important.
      if (newParticle.childIDs.length === 0 && newParticle.hits.length === 0)
        return [];

      return newParticle;
    });

    this.particleMap = new Map();
    this.particles.forEach((particle) => {
      this.particleMap.set(particle.id, particle);
    });

    // The generated property lists...
    this.hitProperties = getHitProperties(this.particles, this.hits);
    this.hitTypes = getHitTypes(this.particles, this.hits);

    // Setup the dynamic bits, the state that will change.
    // This includes the in use hits/markers etc, as well as
    // their types and labels etc...
    this.uiSetup = false;

    // These store the actual hits/markers etc that are in use.
    // This can differ from the static arrays above, as we may
    // only want to show certain hits/markers etc.
    this.activeParticles = [];
    this.activeHits = [];
    this.activeHitColours = [];
    this.activeMC = [];
    this.activeMarkers = [];

    // Similarly, this stores the active properties, which
    // is used to build the active lists above, by filtering
    // the static lists.
    this.activeHitProps = new Set([BUTTON_ID.All]);
    this.activeHitTypes = new Set();
    this.activeMarkerTypes = new Set();
    this.activeInteractionTypes = new Set();
    this.ignoredParticles = new Set();

    // Actually fill the active arrays with their initial values.
    this.#updateActiveArrays();
  }

  /**
   * Renders the detector geometry for the current state. Currently only renders
   * box geometry.
   */
  renderGeometry() {
    this.detGeoGroup.clear();

    // For now, just render the box geometry and nothing else.
    const boxVolumes = this.detectorGeometry.volumes.filter(
      (volume) => volume.volumeType === "box",
    );

    // Since the 2D renderer needs the hits to calculate the box, we need to
    // check if there are any hits, and if not, use the particles instead.
    let hits = this.hits;

    if (hits.length === 0 && this.particles.length > 0) {
      hits = this.particles.flatMap((particle) => particle.hits);
      this.hits = hits;
    }

    boxVolumes.forEach((box) =>
      drawBox(this.hitDim, this.detGeoGroup, hits, box),
    );

    this.detGeoGroup.matrixAutoUpdate = false;
    this.detGeoGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Renders the particles for the current state, based on the active particles.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderParticles() {
    this.hitGroup.clear();

    drawParticles(
      this.hitGroup,
      this.particles,
      this.activeParticles,
      this.activeHitProps,
      this.hitProperties,
      HIT_CONFIG[this.hitDim],
    );

    this.hitGroup.matrixAutoUpdate = false;
    this.hitGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Renders the hits for the current state, based on the active hit types and properties.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderHits() {
    this.hitGroup.clear();

    drawHits(
      this.hitGroup,
      this.activeHits,
      this.activeHitColours,
      HIT_CONFIG[this.hitDim],
    );

    this.hitGroup.matrixAutoUpdate = false;
    this.hitGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Top level event render function, which will render all the different
   * hits of the event, picking between either the particles or the hits.
   */
  renderEvent(fullRender = false) {
    // Update all the active arrays, and check if the
    // number of markers changes.
    const markerNum = this.activeMarkers.length;
    this.#updateActiveArrays();
    const newMarkerNum = this.activeMarkers.length;

    // Render the hits out.
    if (this.particles.length > 0) {
      this.renderParticles();
    } else {
      this.renderHits();
    }

    // Its possible that the marker list has changed, so we need to update
    // the marker UI as well.
    if (markerNum !== newMarkerNum || fullRender) {
      this.renderMarkers();
    }
  }

  /**
   * Renders the MC hits for the current state, based on the active hit types and properties.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderMCHits() {
    this.mcHitGroup.clear();

    const mcColours = getMCColouring(this.activeMC);

    drawHits(
      this.mcHitGroup,
      this.activeMC,
      mcColours,
      HIT_CONFIG[this.hitDim],
    );

    this.mcHitGroup.matrixAutoUpdate = false;
    this.mcHitGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Renders the current markers for the state.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderMarkers() {
    this.markerGroup.clear();

    drawRings(
      this.activeMarkers.filter((marker) => marker.markerType === "Ring"),
      this.markerGroup,
    );
    drawPoints(
      this.activeMarkers.filter((marker) => marker.markerType === "Point"),
      this.markerGroup,
    );

    this.markerGroup.matrixAutoUpdate = false;
    this.markerGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Updates the active hits and hit colours based on the current active hit
   * type and properties.
   */
  #updateHitArrays() {
    const newHits = new Set();
    const newMCHits = [];
    const newHitColours = [];

    // First, do the actual hits...
    this.hits.forEach((hit) => {
      if (
        this.activeHitTypes.size > 0 &&
        !this.activeHitTypes.has(hit.position.hitType)
      )
        return;
      Array.from(this.activeHitProps)
        .reverse()
        .filter((property) => property !== BUTTON_ID.All)
        .forEach((property) => {
          if (!this.hitProperties.get(hit).has(property)) return;
          if (newHits.has(hit)) return;

          newHits.add(hit);
          newHitColours.push(this.hitProperties.get(hit).get(property));
        });

      // If we've already added this hit, we don't need to do anything else.
      if (newHits.has(hit)) return;

      // Otherwise, check if the all button is active, and if so, add it at the end.
      if (this.activeHitProps.has(BUTTON_ID.All)) {
        newHits.add(hit);
        newHitColours.push(this.hitProperties.get(hit).get(BUTTON_ID.All));
      }
    });

    // Then repeat for the MC hits, but skip the hit properties bit.
    this.mcHits.forEach((hit) => {
      if (
        this.activeHitTypes.size > 0 &&
        !this.activeHitTypes.has(hit.position.hitType)
      )
        return;
      newMCHits.push(hit);
    });

    // Finally, update the active particles.
    const newParticles = this.particles.flatMap((particle) => {
      if (
        this.activeInteractionTypes.size > 0 &&
        !this.activeInteractionTypes.has(particle.interactionType)
      )
        return [];

      if (this.ignoredParticles.has(particle.id)) {
        return [];
      }

      const newParticle = { ...particle };

      newParticle.hits = newParticle.hits.filter((hit) => {
        if (
          this.activeHitTypes.size > 0 &&
          !this.activeHitTypes.has(hit.position.hitType)
        )
          return false;

        return Array.from(this.activeHitProps).some((property) => {
          return this.hitProperties.get(hit).has(property);
        });
      });

      if (newParticle.hits.length === 0) return [];

      return newParticle;
    });

    this.activeHits = [...newHits];
    this.activeHitColours = newHitColours;
    this.activeMC = newMCHits;
    this.activeParticles = newParticles;
  }

  /**
   * Similar to the hit arrays, but for the markers.
   * Here, we want only the markers that are active, that are of the active
   * type.
   */
  #updateMarkers() {
    const newMarkers = new Set();

    // Check if there are any active markers, and if not, just return.
    if (this.activeMarkerTypes.size === 0) {
      this.activeMarkers = [];
      return;
    }

    // Otherwise, loop over the markers and add them if they're active.
    this.activeMarkerTypes.forEach((markerType) => {
      this.markers.forEach((marker) => {
        if (
          this.activeHitTypes.size > 0 &&
          !this.activeHitTypes.has(marker.position.hitType)
        )
          return;
        if (marker.markerType === markerType) newMarkers.add(marker);
      });
    });

    // If there are no markers still, but there are particles
    // we want to add the vertex markers.
    if (this.particles.length > 0) {
      this.particles.forEach((particle) => {
        if (
          this.activeInteractionTypes.size > 0 &&
          !this.activeInteractionTypes.has(particle.interactionType)
        )
          return;

        if (this.ignoredParticles.has(particle.id)) {
          return;
        }

        particle.vertices.forEach((vertex) => {
          if (
            this.activeHitTypes.size > 0 &&
            !this.activeHitTypes.has(vertex.position.hitType)
          )
            return;

          newMarkers.add(vertex);
        });
      });
    }

    this.activeMarkers = [...newMarkers];
  }

  /*
   * Run all the update functions for the active arrays.
   */
  #updateActiveArrays() {
    this.#updateHitArrays();
    this.#updateMarkers();
  }

  // What to do if the hit property option changes:
  //  - Update the active hit properties list.
  //  - We need to update any UI around them.
  //  - Can then re-render the hits out.
  onHitPropertyChange(hitProperty) {
    const buttonActive = isButtonActive(this.hitDim, hitProperty);
    const sceneActive = this.scene.visible;

    // If there was no actual hitProperty, or the scene isn't active but the
    // hits are setup correctly, just swap the scene and finish.
    if (hitProperty === "" || (buttonActive && !sceneActive)) {
      this.toggleScene(this.hitDim);
      return;
    }

    // If the "None" property is clicked, we want to toggle everything off.
    // Otherwise, add or remove this property from the list.
    if (hitProperty === BUTTON_ID.None) {
      this.activeHitProps.clear();
    } else {
      // Add or remove the toggled property as needed...
      if (this.activeHitProps.has(hitProperty)) {
        this.activeHitProps.delete(hitProperty);
      } else {
        this.activeHitProps.add(hitProperty);
      }
    }

    // Now that the internal state is correct, correct the UI.
    toggleButton(this.hitDim, hitProperty);
    this.toggleScene(this.hitDim);

    // Finally, render the event hits!
    this.renderEvent();
  }

  // Similar to the property change, update the hit type list.
  onHitTypeChange(hitType) {
    // Add or remove the toggled class as needed...
    if (this.activeHitTypes.has(hitType)) {
      this.activeHitTypes.delete(hitType);
    } else {
      this.activeHitTypes.add(hitType);
    }

    // Now that the internal state is correct, correct the UI.
    toggleButton("types", hitType, false);
    this.toggleScene(this.hitDim);

    // Finally, render the event hits!
    this.renderEvent();
  }

  // If any markers are toggled, update the list.
  onMarkerChange(markerType) {
    // Add or remove the toggled class as needed...
    if (this.activeMarkerTypes.has(markerType)) {
      this.activeMarkerTypes.delete(markerType);
    } else {
      this.activeMarkerTypes.add(markerType);
    }

    // Fix the active markers for this change...
    this.#updateMarkers();

    // Now that the internal state is correct, correct the UI.
    toggleButton(`markers_${this.hitDim}`, markerType, false);
    this.toggleScene(this.hitDim);

    // Finally, render the new markers!
    this.renderMarkers();
  }

  // If any particle interaction types are toggled, update the list.
  onInteractionTypeChange(interactionType) {
    // Add or remove the toggled class as needed...
    if (this.activeInteractionTypes.has(interactionType)) {
      this.activeInteractionTypes.delete(interactionType);
    } else {
      this.activeInteractionTypes.add(interactionType);
    }

    // Now that the internal state is correct, correct the UI.
    toggleButton(`particles_${this.hitDim}`, interactionType, false);

    // Finally, render the event hits!
    this.renderEvent();
  }

  // Finally, a MC-hit based toggle, enabling or disabling as needed.
  onMCToggle() {
    // Toggle the visibility state.
    this.mcHitGroup.visible = !this.mcHitGroup.visible;

    // Now that the internal state is correct, correct the UI.
    toggleButton("types_MC_toggle", this.hitDim, false);

    // Render out the new hits.
    this.renderMCHits();
  }

  // Setup the UI, should only be called once.
  // Populate various drop downs and buttons based on the state,
  // and then reset the camera.
  setupUI(renderTarget, resetUI = false) {
    if (this.uiSetup) return;

    this.renderGeometry();
    this.renderEvent();

    // Fill in any dropdown entries, or hit class toggles.
    populateDropdown(this.hitDim, this.hitProperties, (prop) =>
      this.onHitPropertyChange(prop),
    );
    populateTypeToggle(this.hitDim, this.hitTypes, (hitType) =>
      this.onHitTypeChange(hitType),
    );
    populateMarkerToggle(
      this.hitDim,
      this.markers,
      this.particles,
      (markerType) => this.onMarkerChange(markerType),
    );
    enableMCToggle(this.hitDim, this.mcHits, () => this.onMCToggle());
    enableInteractionTypeToggle(
      this.hitDim,
      this.particles,
      (interactionType) => this.onInteractionTypeChange(interactionType),
    );

    // Move the scene/camera around to best fit it in.
    if (! resetUI) {
        fitSceneInCamera(this.camera, this.controls, this.detGeoGroup, this.hitDim);
        setupControls(this.hitDim, this.controls);
        this.scene.add(this.camera);
    }

    // Setup the default button.
    toggleButton(this.hitDim, BUTTON_ID.All);
    setupParticleMenu(this);

    this.toggleScene(renderTarget);
    this.uiSetup = true;
  }

  // Attempt to activate or deactivate the scene, if needed.
  toggleScene(renderTarget) {
    this.scene.visible = this.hitDim === renderTarget;
    this.controls.enabled = this.hitDim === renderTarget;

    this.otherRenderer.scene.visible =
      this.otherRenderer.hitDim === renderTarget;
    this.otherRenderer.controls.enabled =
      this.otherRenderer.hitDim === renderTarget;

    this.triggerEvent("change");
    updateUI(renderTarget);
  }

  // If this is currently active, reset the event display.
  resetView() {
    if (!this.scene.visible) return;

    // Reset the camera + controls.
    this.controls.reset();

    fitSceneInCamera(this.camera, this.controls, this.detGeoGroup, this.hitDim);
  }
}
