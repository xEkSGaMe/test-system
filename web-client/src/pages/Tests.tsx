import { useEffect, useState } from "react";

export default function Tests({ token }: { token: string }) {
  const [tests, setTests] = useState([]);

  useEffect(() => {
    if (!token) return;

    fetch("http://localhost:8080/tests", {
      headers: {
        Authorization: `Bearer ${token}`,
      },
    })
      .then((r) => r.json())
      .then((data) => setTests(data))
      .catch((err) => console.error(err));
  }, [token]);

  if (!token) return <p>You must log in first.</p>;

  return (
    <div>
      <h1>Tests</h1>
      <ul>
        {tests.map((t: any) => (
          <li key={t.id}>{t.title}</li>
        ))}
      </ul>
    </div>
  );
}
