<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <title>WebSocket Client</title>
</head>

<body>
    <h1>WebSocket Client In JabaSclipt</h1>
    <button id="connectButton">Connect</button>
    <button id="sendButton" disabled>Send Message</button>
    <input type="text" id="messageInput" placeholder="Enter message" disabled>
    <pre id="output"></pre>

    <script>
        let socket;

        document.getElementById('connectButton').addEventListener('click', () => {
            socket = new WebSocket('ws://172.25.128.7:8080');

            socket.addEventListener('open', (event) => {
                document.getElementById('output').textContent += 'Connected to the WebSocket server\n';
                document.getElementById('sendButton').disabled = false;
                document.getElementById('messageInput').disabled = false;
                document.getElementById('fileInput').disabled = false;
                document.getElementById('sendImageButton').disabled = false;
            });

            socket.addEventListener('message', (event) => {
                // Check if the received message is a Blob (image)
                if (event.data instanceof Blob) {
                    const img = new Image();
                    const url = URL.createObjectURL(event.data);
                    img.src = url;
                    document.getElementById('output').appendChild(img);
                    document.getElementById('output').appendChild(document.createElement('br'));
                } else {
                    // Try parsing as JSON
                    try {
                        const data = JSON.parse(event.data);
                        if (data.type === 'image') {
                            const img = new Image();
                            img.src = data.content;
                            document.getElementById('output').appendChild(img);
                            document.getElementById('output').appendChild(document.createElement('br'));
                        } else {
                            document.getElementById('output').textContent += 'Message from server: ' + event.data + '\n';
                        }
                    } catch {
                        // If not JSON, treat as regular message
                        document.getElementById('output').textContent += 'Message from server: ' + event.data + '\n';
                    }
                }
            });
            
            socket.addEventListener('close', (event) => {
                document.getElementById('output').textContent += 'Disconnected from the WebSocket server\n';
                document.getElementById('sendButton').disabled = true;
                document.getElementById('messageInput').disabled = true;
                document.getElementById('fileInput').disabled = true;
                document.getElementById('sendImageButton').disabled = true;
            });

            // Event listener for when there is an error with the connection
            socket.addEventListener('error', (event) => {
                document.getElementById('output').textContent += 'WebSocket error: ' + event.message + '\n';
            });
        });

        document.getElementById('sendButton').addEventListener('click', () => {
            const message = document.getElementById('messageInput').value;
            if (socket && socket.readyState === WebSocket.OPEN) {
                socket.send(message);
                document.getElementById('output').textContent += 'Sent: ' + message + '\n';
                document.getElementById('messageInput').value = 'x';
            }
        });
    </script>
</body>

</html>
