name = "Quickshell.Services.Polkit"
description = "Polkit Agent"
headers = [
	"agentimpl.hpp",
	"flow.hpp",
	"identity.hpp",
	"listener.hpp",
	"qml.hpp",
	"session.hpp",
]
-----
## Purpose of a Polkit Agent

PolKit is a system for privileged applications to query if a user is permitted to execute an action.
You have probably seen it in the form of a "Please enter your password to continue with X" dialog box before.
This dialog box is presented by your *PolKit agent*, it is a process running as your user that accepts authentication requests from the *daemon* and presents them to you to accept or deny.

This service enables writing a PolKit agent in Quickshell.

## Implementing a Polkit Agent

The backend logic of communicating with the daemon is handled by the @@Quickshell.Services.Polkit.PolkitAgent object.
It exposes incoming requests via @@Quickshell.Services.Polkit.PolkitAgent.flow and provides appropriate signals.

### Flow of an authentication request

Incoming authentication requests are queued in the order that they arrive.
If none is queued, a request starts processing right away.
Otherwise, it will wait until prior requests are done.

A request starts by emitting the @@Quickshell.Services.Polkit.PolkitAgent.authenticationRequestStarted signal.
At this point, information like the action to be performed and permitted users that can authenticate is available.

An authentication *session* for the request is immediately started, which internally starts a PAM conversation that is likely to prompt for user input.
* Additional prompts may be shared with the user by way of the @@Quickshell.Services.Polkit.AuthFlow.supplementaryMessageChanged / @@Quickshell.Services.Polkit.AuthFlow.supplementaryIsErrorChanged signals and the @@Quickshell.Services.Polkit.AuthFlow.supplementaryMessage and @@Quickshell.Services.Polkit.AuthFlow.supplementaryIsError properties. A common message might be 'Please input your password'.
* An input request is forwarded via the @@Quickshell.Services.Polkit.AuthFlow.isResponseRequiredChanged / @@Quickshell.Services.Polkit.AuthFlow.inputPromptChanged / @@Quickshell.Services.Polkit.AuthFlow.responseVisibleChanged signals and the corresponding properties. Note that the request specifies whether the text box should show the typed input on screen or replace it with placeholders.

User replies can be submitted via the @@Quickshell.Services.Polkit.AuthFlow.submit method.
A conversation can take multiple turns, for example if second factors are involved.

If authentication fails, we automatically create a fresh session so the user can try again.
The @@Quickshell.Services.Polkit.AuthFlow.authenticationFailed signal is emitted in this case.

If authentication is successful, you receive the @@Quickshell.Services.Polkit.AuthFlow.authenticationSucceeeded signal. At this point, the dialog can be closed.
If additional requests are queued, you will receive the @@Quickshell.Services.Polkit.PolkitAgent.authenticationRequestStarted signal again.

#### Cancelled requests

Requests may either be canceled by the user or the PolKit daemon.
In this case, we clean up any state and proceed to the next request, if any.

If the request was cancelled by the daemon and not the user, you also receive the @@Quickshell.Services.Polkit.AuthFlow.authenticationRequestCancelled signal.
