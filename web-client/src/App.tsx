import { BrowserRouter, Routes, Route, Navigate } from "react-router-dom";
import { useEffect, useState } from "react";

import Header from "./components/Header";

import Home from "./pages/Home";
import Tests from "./pages/Tests";
import TestView from "./pages/TestView";
import Dashboard from "./pages/Dashboard";
import Admin from "./pages/Admin";

export default function App() {
  const [token, setToken] = useState(localStorage.getItem("token") || "");

  useEffect(() => {
    const params = new URLSearchParams(window.location.search);
    const t = params.get("token");
    if (t) {
      localStorage.setItem("token", t);
      setToken(t);
      window.history.replaceState({}, document.title, "/");
    }
  }, []);

  return (
    <BrowserRouter>
      <Header token={token} setToken={setToken} />

      <Routes>
        <Route path="/" element={<Home token={token} />} />
        <Route path="/tests" element={<Tests token={token} />} />
        <Route path="/test/:id" element={<TestView token={token} />} />
        <Route path="/dashboard" element={<Dashboard token={token} />} />
        <Route path="/admin" element={<Admin token={token} />} />
        <Route path="*" element={<Navigate to="/" />} />
      </Routes>
    </BrowserRouter>
  );
}
