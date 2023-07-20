//
// GUI Functions
//

import * as THREE from "three";

import { BUTTON_ID, HIT_CONFIG, materialHit } from "./constants.js";
import { drawHits } from "./hits.js";

// Given a new hit-based property to toggle (toggleTarget), either
// toggle it's visibility, or render a new group for it.
// If the button is the none button, instead set all the groups off.
//
// We have to be a bit creative in how we do the rendering. Rendering of so
// many (order thousands) of tiny cubes is only fast due to using the
// THREE.InstancedMesh. However, we ideally want a singular instanced mesh
// with every single hit in, rather than many instanced meshes.
//
// For that reason, we build up a superset based on all the currently active
// targets, and then render out a single InstancedMesh that contains all the
// hits from all currently active properties.
//
// Builds up a cache string based on all the currently active properties
// to utilise older InstancedMesh instances.
export function hitsToggle(
  allHits,
  activeHits,
  hitGroupMap,
  hitPropMap,
  classFilter,
  hitConfig,
  toggleTarget,
) {
  if (toggleTarget === BUTTON_ID.None) {
    hitGroupMap.forEach((group) => (group.visible = false));
    activeHits.clear();
    return;
  }

  const currentKey = [...activeHits.keys()].sort().join("_");

  if (hitGroupMap.has(currentKey)) {
    const threeDHitGroup = hitGroupMap.get(currentKey);
    threeDHitGroup.visible = false;
  }

  // Add/Remove the hits from the activeHits map as needed.
  if (activeHits.has(toggleTarget)) {
    activeHits.delete(toggleTarget);
  } else {
    const newHitsToRender = [];
    allHits.forEach((hit) => {
      if (!hitPropMap.has(hit)) {
        return;
      }

      if (!hitPropMap.get(hit).has(toggleTarget)) {
        return;
      }

      newHitsToRender.push(hit);
    });
    activeHits.set(toggleTarget, newHitsToRender);
  }

  const newKey = [...activeHits.keys()].sort().join("_");

  if (newKey.length === 0) {
    return;
  }

  // If the current combination exists, just toggle it and return.
  if (hitGroupMap.has(newKey)) {
    const threeDHitGroup = hitGroupMap.get(newKey);
    threeDHitGroup.visible = true;
    return;
  }

  // Otherwise, we need to make a new group, populate it and store it for later.
  const newGroup = new THREE.Group();
  drawHits(newGroup, materialHit, activeHits, hitPropMap, true, hitConfig, classFilter);
  hitGroupMap.set(newKey, newGroup);

  return newGroup;
}

// Given a drop down, populate it with all the available options for that
// renderer. Options are based on the labels and properties of the hits.
export function populateDropdown(className, hitPropMap, onClick = (_) => {}) {
  const dropDown = document.getElementById(`${className}_dropdown`);
  const entries = new Set();

  // Add the default "None" option.
  entries.add(BUTTON_ID.None);

  if (hitPropMap.size != 0) entries.add(BUTTON_ID.All);

  hitPropMap.forEach((properties, _) => {
    properties.forEach((_, propString) => entries.add(propString));
  });

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.innerText = entry;
    newButton.id = `${className}_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    dropDown.appendChild(newButton);
  });

  return;
}

// Populate and enable the hit class toggles.
// This is based on the hit class (say MC vs Normal, or different 2D projections),
// rather than any labels.
export function populateClassToggle(className, hits, onClick = (_) => {}) {
  const classDiv = document.getElementById(`classes_${className}`);
  const entries = new Set();

  hits.forEach((hit, _) => entries.add(hit.class));

  // If there is no entries, or only the default "Hit" class, don't bother.
  if (entries.size <= 1) {
    return;
  }

  entries.forEach((entry) => {
    const newButton = document.createElement("button");
    newButton.innerText = entry;
    newButton.id = `classes_${entry}`;
    newButton.addEventListener("click", () => onClick(entry));
    classDiv.appendChild(newButton);
  });

  return;
}

// Toggle active state of a given button.
//
// If that button is the "None" button, we should also
// toggle the state of every other button in that dropdown.
// Similarly, if its not that none button, toggle the none
// button off.
export function toggleButton(className, ID, fixNoneButton = true) {
  const button = document.getElementById(`${className}_${ID}`);

  let isActive = button.style.color === "white";

  if (isActive) {
    button.style.color = "green";
    isActive = false;
  } else {
    button.style.color = "white";
    isActive = true;
  }

  if (!fixNoneButton)
    return;

  if (ID === BUTTON_ID.None && isActive) {
    const dropDown = document.getElementById(`${className}_dropdown`);

    Array.from(dropDown.childNodes)
      .filter(
        (elem) =>
          elem.nodeName != "#text" &&
          elem != button &&
          elem.tagName.toLowerCase() === "button",
      )
      .forEach((elem) => {
        elem.style.color = "green";
      });
  } else if (ID !== BUTTON_ID.None && isActive) {
    const button = document.getElementById(`${className}_${BUTTON_ID.None}`);
    button.style.color = "green";
  }
}

// Is the given button active?
export function isButtonActive(className, ID) {
  const button = document.getElementById(`${className}_${ID}`);
  return button.style.color === "white";
}

// Update the UI for swapping between any options.
export function updateUI(className) {
  const toggleOptions = document.getElementById("all_toggle_options");
  Array.from(toggleOptions.childNodes)
    .filter((elem) => elem.nodeName != "#text")
    .forEach((elem) => {
      // Toggle visibility for the new class.
      if (elem.id === `classes_${className}`) {
        elem.style.display = "block";
      } else {
        elem.style.display = "none";
      }
    });
}

export function saveEvd(renderer) {
  const imageData = renderer.domElement.toDataURL("image/jpeg", 1.0);
  const contentType = "image/jpeg";

  const byteCharacters = atob(
    imageData.substr(`data:${contentType};base64,`.length),
  );
  const bytes = [];

  for (let offset = 0; offset < byteCharacters.length; offset += 1024) {
    const slice = byteCharacters.slice(offset, offset + 1024);
    const byteNumbers = new Array(slice.length);
    for (let i = 0; i < slice.length; i++) {
      byteNumbers[i] = slice.charCodeAt(i);
    }

    const byteArray = new Uint8Array(byteNumbers);
    bytes.push(byteArray);
  }

  const blob = new Blob(bytes, { type: contentType });
  const blobUrl = URL.createObjectURL(blob);

  window.open(blobUrl, "_blank");
}
