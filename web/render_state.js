//
// Rendering State
//

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

import { fitSceneInCamera, setupControls } from "./camera_and_controls.js";
import { getCategoricalLutConf, getContinuousLutConf } from "./colourmaps.js";
import { BUTTON_ID, HIT_CONFIG } from "./constants.js";
import { getMCColouring } from "./helpers.js";
import { HitDataState } from "./hit_data_state.js";
import { HitTypeState } from "./hit_type_state.js";
import { drawHits, drawParticles } from "./hits.js";
import { MarkerDataState } from "./marker_data_state.js";
import { drawLines, drawPoints, drawRingMarker } from "./markers.js";
import { MCDataState } from "./mc_data_state.js";
import { ParticleDataState } from "./particle_data_state.js";
import { drawBox, drawTrapezoids } from "./rendering.js";
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
    stateInfo,
  ) {
    // Basic, crucial information...
    this.name = name;
    this.hitDim = name;

    // THREE.js Setup...
    this.scene = new THREE.Scene();
    this.camera = camera;
    this.controls = new OrbitControls(this.camera, renderer.domElement);

    // Setup various groups for rendering into...
    const getGroup = () => {
      const group = new THREE.Group();
      group.matrixAutoUpdate = false;
      group.matrixWorldAutoUpdate = false;

      return group;
    };

    this.detGeoGroup = getGroup();
    this.hitGroup = getGroup();
    this.mcHitGroup = getGroup();
    this.markerGroup = getGroup();

    // Initial setup of the groups...
    this.mcHitGroup.visible = false;

    // Setup the data...
    this.updateData(particles, hits, mcHits, markers, geometry, stateInfo);

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
    if (this.particleData.length > 0) return this.particleData.length;
    return this.hitData.length;
  }

  /**
   * Returns a boolean indicating whether the scene is currently visible.
   * @returns {boolean} Whether the scene is visible.
   */
  get visible() {
    return this.scene.visible;
  }

  /**
   * Return the scene size, collated from all the individual groups.
   * @returns {number} The size of the scene.
   */
  get sceneSize() {
    return (
      this.detGeoGroup.children.length +
      this.hitGroup.children.length +
      this.mcHitGroup.children.length +
      this.markerGroup.children.length
    );
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
   * @param {Object} stateInfo - High level state information.
   */
  updateData(particles, hits, mcHits, markers, geometry, stateInfo) {
    // Data Setup, first the top level static arrays...
    this.detectorGeometry = geometry;
    this.stateInfo = stateInfo;

    // Filter the particles to only those that have hits in the current
    // dimension.
    const filteredParticles = particles.flatMap((particle) => {
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

    // Setup the controls, since it varies depending on the view type.
    this.controlsSetups = false;

    // These store the actual hits/markers etc that are in use.
    // This can differ from the static arrays above, as we may
    // only want to show certain hits/markers etc.
    this.hitTypeState = new HitTypeState(filteredParticles, hits);
    this.particleData = new ParticleDataState(filteredParticles);
    this.hitData = new HitDataState(filteredParticles, hits);
    this.mcData = new MCDataState(mcHits);
    this.markerData = new MarkerDataState(markers);

    // Actually fill the active arrays with their initial values.
    this.#updateActiveArrays();
  }

  /**
   * Renders the detector geometry for the current state. Currently only renders
   * box geometry.
   */
  renderGeometry() {
    this.detGeoGroup.clear();

    // First, render the box volumes.
    this.detectorGeometry.volumes
      .filter((volume) => volume.volumeType === "box")
      .forEach((box) => {
        drawBox(this.hitDim, this.detGeoGroup, box);
      });

    // Next, any trapezoid volumes.
    if (this.hitDim === "3D") {
      drawTrapezoids(
        this.detGeoGroup,
        this.detectorGeometry.volumes.filter(
          (volume) => volume.volumeType === "trapezoid",
        ),
      );
      drawTrapezoids(
        this.detGeoGroup,
        this.detectorGeometry.volumes.filter(
          (volume) => volume.volumeType === "rectangle2D",
        ),
      );
    }

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

    // INFO: Duplicate the hitConfig, so we can modify it for particles.
    const hitConfig = Object.assign({}, HIT_CONFIG[this.hitDim]);
    drawParticles(this.hitGroup, this.particleData, this.hitData, hitConfig);

    this.hitGroup.matrixAutoUpdate = false;
    this.hitGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Renders the hits for the current state, based on the active hit types and properties.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderHits(
    hits = this.hitData.hits,
    colours = this.hitData.colours,
    clear = true,
  ) {
    if (clear) this.hitGroup.clear();

    // Use the continuous colouring by default.
    let lutConfig = getContinuousLutConf();

    // If any of the active hit properties are categorical, use that instead.
    if (this.hitData.activeProps.size > 0) {
      this.hitData.activeProps.forEach((prop) => {
        if (this.hitData.propTypes.get(prop) === "CATEGORIC") {
          lutConfig = getCategoricalLutConf();
        }
      });
    }

    // INFO: Duplicate the hitConfig, so we can modify it for hits.
    const hitConfig = Object.assign({}, HIT_CONFIG[this.hitDim]);
    drawHits(this.hitGroup, hits, colours, hitConfig, lutConfig);

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
    const markerNum = this.markerData.length;
    this.#updateActiveArrays();
    const newMarkerNum = this.markerData.length;

    // Render the hits out.
    if (this.particleData.length > 0) {
      this.renderParticles();
    } else {
      this.renderHits();
    }

    // Its possible that the marker list has changed, so we need to update
    // the marker UI as well.
    if (markerNum !== newMarkerNum || fullRender) {
      this.renderMarkers();
    }

    // Similarly, if the MC hits are active, we need to render them out.
    if (this.mcHitGroup.visible) {
      this.renderMCHits();
    }
  }

  /**
   * Renders the MC hits for the current state, based on the active hit types and properties.
   * Clears the hit group and then draws the hits with the active hit colours.
   */
  renderMCHits() {
    this.mcHitGroup.clear();

    const mcColours = getMCColouring(this.mcData.mc);
    const hitConfig = Object.assign({}, HIT_CONFIG[this.hitDim]);

    if (this.hitDim === "3D") {
      hitConfig.hitSize += 0.5;
    } else {
      hitConfig.renderOrder = 500;
    }

    drawHits(this.mcHitGroup, this.mcData.mc, mcColours, hitConfig);

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

    drawRingMarker(this.markerData.getMarkersOfType("Ring"), this.markerGroup);
    drawPoints(this.markerData.getMarkersOfType("Point"), this.markerGroup);
    drawLines(this.markerData.getMarkersOfType("Line"), this.markerGroup);

    this.markerGroup.matrixAutoUpdate = false;
    this.markerGroup.matrixWorldAutoUpdate = false;
    this.triggerEvent("change");
  }

  /**
   * Updates the active hits and hit colours based on the current active hit
   * type and properties.
   */
  #updateHitArrays() {
    this.particleData.updateActive(this.hitData, this.hitTypeState);
    this.hitData.updateActive(this.particleData.particles, this.hitTypeState);
    this.mcData.updateActive(this.hitTypeState);
  }

  /**
   * Similar to the hit arrays, but for the markers.
   * Here, we want only the markers that are active, that are of the active
   * type.
   */
  #updateMarkers() {
    this.markerData.updateActive(
      this.particleData.particles,
      this.hitTypeState,
    );
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

    // Add or remove the toggled class as needed...
    this.hitData.toggleHitProperty(hitProperty);

    // Now that the internal state is correct, correct the UI.
    toggleButton(this.hitDim, hitProperty);
    this.toggleScene(this.hitDim);

    // Finally, render the event hits!
    this.renderEvent();
  }

  // Similar to the property change, update the hit type list.
  onHitTypeChange(hitType) {
    // If this is the none button, just remove all the hits.
    // This is a special case, as we don't want to toggle the
    // hit type state -> Its possible the user wants to see
    // no hits, but only X hit type markers.
    if (hitType === BUTTON_ID.None) {
      this.hitData.toggleActive();
      this.particleData.toggleActive();
    } else {
      // Add or remove the toggled class as needed...
      this.hitTypeState.toggleHitType(hitType);
    }

    // Now that the internal state is correct, correct the UI.
    toggleButton("types", hitType, false);
    this.toggleScene(this.hitDim);

    // Finally, render the event hits!
    this.renderEvent();
  }

  // If any markers are toggled, update the list.
  onMarkerChange(markerType) {
    // Fix the active markers for this change...
    this.markerData.toggleMarkerType(markerType);
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
    this.particleData.toggleInteractionType(interactionType);

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
  async setupUI(renderTarget, resetUI = false) {
    // If nothing to render, just return after hiding the dropdown button.
    const dropDownButton = document.getElementById(
      `${this.hitDim}_dropdown_button`,
    );
    if (this.hitData.all.length === 0 && this.particleData.all.length === 0) {
      dropDownButton.style.display = "none";

      return;
    } else {
      dropDownButton.style.display = "grid";
    }

    this.renderGeometry();
    this.renderEvent();

    // Fill in any dropdown entries, or hit class toggles.
    populateDropdown(this.hitDim, this.hitData.props, (prop) =>
      this.onHitPropertyChange(prop),
    );
    populateTypeToggle(this.hitDim, this.hitTypeState.types, (hitType) =>
      this.onHitTypeChange(hitType),
    );

    // And the marker toggles...
    populateMarkerToggle(
      this.hitDim,
      this.markerData.markers,
      this.particleData.particles,
      (markerType) => this.onMarkerChange(markerType),
    );

    // And finally the MC and interaction type toggles.
    enableMCToggle(this.hitDim, this.mcData.mc, () => this.onMCToggle());

    // We don't want anything to be visible by default...
    this.mcHitGroup.visible = false;

    enableInteractionTypeToggle(
      this.hitDim,
      this.particleData.particles,
      (interactionType) => this.onInteractionTypeChange(interactionType),
    );

    // Move the scene/camera around to best fit it in.
    if (!resetUI) {
      fitSceneInCamera(
        this.camera,
        this.controls,
        this.detGeoGroup,
        this.hitDim,
      );
      this.scene.add(this.camera);
    }

    if (!this.controlsSetups) {
      setupControls(this.hitDim, this.controls);
      this.controlsSetups = true;
    }

    // Setup the default button.
    toggleButton(this.hitDim, BUTTON_ID.All);
    setupParticleMenu(this);

    this.toggleScene(renderTarget);
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
    updateUI(renderTarget, this.stateInfo.mcTruth);
  }

  // If this is currently active, reset the event display.
  resetView() {
    if (!this.scene.visible) return;

    // Reset the camera + controls.
    this.controls.reset();

    fitSceneInCamera(this.camera, this.controls, this.detGeoGroup, this.hitDim);
  }
}
