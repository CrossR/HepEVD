//
// Event / state management for the web interface.
//

export async function updateStateUI() {
  const stateIdPairs = await getAllStateInfo();
  const stateDiv = document.getElementById("stateSwapper");

  // Only show the state swapper if there are multiple states.
  if (stateIdPairs.length == 1) {
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
    newButton.addEventListener("click", () => setState(idStatePair.id));
    listElement.appendChild(newButton);
    stateList.appendChild(listElement);
  });

  const currentState = await getCurrentStateInfo();
  const currentStateButton = document.getElementById(`state_dropdown_button`);

  currentStateButton.innerText = currentState.name;
}

export function getCurrentStateInfo() {
  return fetch("/stateInfo").then((response) => response.json());
}

export function getAllStateInfo() {
  return fetch("/allStateInfo").then((response) => response.json());
}

// Swa to the given state.
export function setState(stateId) {
  fetch(`/swap/id/${stateId}`);
  updateStateUI();
}

// Swap the data source to the next event / state.
export function nextState() {
  // TODO: Extend this to work with GitHub Gist URLs.
  //       Probably need some form or list of URLs to iterate through.

  fetch("/nextState");
  updateStateUI();
}

// Swap the data source to the previous event / state.
export function previousState() {
  // TODO: Extend this to work with GitHub Gist URLs.
  //       Probably need some form or list of URLs to iterate through.

  fetch("/previousState");
  updateStateUI();
}
