//
// Event / state management for the web interface.
//

// TODO: Won't work on GitHub pages, since it's not a server.
//       Need to figure out best way to supply multiple states there.

import { getData, hepEVD_GLOBAL_STATE } from "./data_loader.js";
import { populateImages } from "./ui.js";

/**
 * Updates the UI for the state swapper based on the current state.
 *
 * @param {Function} renderStates - Map of render states.
 * @returns {Promise<void>} - A Promise that resolves when the UI has been updated.
 */
export async function updateStateUI(renderStates) {
  const stateDiv = document.getElementById("state_swapper");

  const stateIdPairs = await getAllStateInfo();

  // Only show the state swapper if there are multiple states.
  if (stateIdPairs === null || stateIdPairs.length === 1) {
    stateDiv.style.display = "none";
    return;
  }

  // Since there are multiple states, populate the state swapper.
  const stateList = document.getElementById("state_dropdown");

  // Clear out the old states.
  while (stateList.firstChild) {
    stateList.removeChild(stateList.firstChild);
  }

  // Add the new states.
  stateIdPairs.forEach((idStatePair, index) => {
    const state = idStatePair.state;
    const listElement = document.createElement("li");
    const newButton = document.createElement("li");
    newButton.style.textTransform = "capitalize";
    newButton.innerText = state.name;
    newButton.id = `state_${state.name}_${idStatePair.id}`;
    newButton.addEventListener("click", () =>
      setState(idStatePair.id, renderStates),
    );
    listElement.appendChild(newButton);
    stateList.appendChild(listElement);
  });

  const currentState = await getCurrentStateInfo();
  const currentStateButton = document.getElementById(`state_dropdown_button`);
  currentStateButton.innerText = currentState.name;

  // Update the next/previous buttons, if we are at the start or end of the list.
  const previousButton = document.getElementById("previous_state");
  const nextButton = document.getElementById("next_state");

  previousButton.disabled = currentState.name === stateIdPairs[0].state.name;
  nextButton.disabled =
    currentState.name === stateIdPairs.slice(-1)[0].state.name;
}

export async function reloadDataForCurrentState(renderStates) {
  const data = await getData();
  const {
    hits,
    mcHits,
    markers,
    particles,
    images,
    detectorGeometry,
    stateInfo,
  } = data;

  renderStates.forEach((state) => {
    state.updateData(
      particles,
      hits.filter((hit) => hit.position.dim === state.hitDim),
      mcHits.filter((hit) => hit.position.dim === state.hitDim),
      markers.filter((marker) => marker.position.dim === state.hitDim),
      detectorGeometry,
      stateInfo,
    );
  });

  const currentView = renderStates.get("3D").visible ? "3D" : "2D";
  let drawTarget = currentView;

  // If the current view is empty, swap to the other view.
  if (currentView === "3D" && renderStates.get("3D").hitSize === 0) {
    drawTarget = "2D";
  } else if (currentView === "2D" && renderStates.get("2D").hitSize === 0) {
    drawTarget = "3D";
  }

  renderStates.forEach((state) => {
    state.setupUI(drawTarget, true);
    state.triggerEvent("fullUpdate");
  });

  // Update the images UI, hiding it if needed.
  populateImages(images);
}

/**
 * Retrieves the current state information from the server.
 *
 * @returns {Promise} A Promise that resolves with the JSON state information.
 */
export function getCurrentStateInfo() {
  if (hepEVD_GLOBAL_STATE !== undefined) {
    return Promise.resolve(
      hepEVD_GLOBAL_STATE.states[hepEVD_GLOBAL_STATE.currentState],
    );
  }

  return fetch("/stateInfo").then((response) => response.json());
}

/**
 * Fetches all state information.
 *
 * @returns {Promise} A Promise that resolves to a map of state IDs to state.
 */
export function getAllStateInfo() {
  if (hepEVD_GLOBAL_STATE !== undefined) {
    const stateIdPairs = [];
    hepEVD_GLOBAL_STATE.states.forEach((state, index) => {
      stateIdPairs.push({ id: index, state });
    });

    return Promise.resolve(stateIdPairs);
  }

  return fetch("/allStateInfo").then((response) => response.json());
}

/**
 * Sets the state of the application to the given state ID.
 *
 * @param {number} stateId - The ID of the state to set.
 * @param {Function} renderStates - Map of render states.
 */
export async function setState(stateId, renderStates) {
  if (hepEVD_GLOBAL_STATE !== undefined) {
    hepEVD_GLOBAL_STATE.currentState = stateId;
  } else {
    await fetch(`/swap/id/${stateId}`);
  }

  updateStateUI(renderStates);
  reloadDataForCurrentState(renderStates);
}

/**
 * Fetches the next state and updates the UI accordingly.
 *
 * @param {Function} renderStates - Map of render states.
 *
 * @returns {void}
 */
export async function nextState(renderStates) {
  if (hepEVD_GLOBAL_STATE !== undefined) {
    hepEVD_GLOBAL_STATE.currentState =
      (hepEVD_GLOBAL_STATE.currentState + 1) %
      hepEVD_GLOBAL_STATE.states.length;
  } else {
    await fetch("/nextState");
  }

  updateStateUI(renderStates);
  reloadDataForCurrentState(renderStates);
}

/**
 * Fetches the previous state and updates the UI accordingly.
 *
 * @param {Function} renderStates - Map of render states.
 *
 * @returns {void}
 */
export async function previousState(renderStates) {
  if (hepEVD_GLOBAL_STATE !== undefined) {
    hepEVD_GLOBAL_STATE.currentState =
      (hepEVD_GLOBAL_STATE.currentState -
        1 +
        hepEVD_GLOBAL_STATE.states.length) %
      hepEVD_GLOBAL_STATE.states.length;
  } else {
    await fetch("/previousState");
  }
  updateStateUI(renderStates);
  reloadDataForCurrentState(renderStates);
}
