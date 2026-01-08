import { Link, useNavigate } from "react-router-dom";
import { useEffect, useState } from "react";
import "./Header.css";

export default function Header({ token, setToken }: { token: string; setToken: (t: string) => void }) {
  const navigate = useNavigate();
  const [role, setRole] = useState("");

  useEffect(() => {
    if (!token) return;
    fetch("http://localhost:8081/me", {
      headers: { Authorization: `Bearer ${token}` },
    })
      .then((r) => r.json())
      .then((data) => {
        if (data.success) setRole(data.data.role);
      })
      .catch(() => {});
  }, [token]);

  const logout = () => {
    localStorage.removeItem("token");
    setToken("");
    navigate("/");
  };

  return (
    <header className="header">
      <nav className="navbar">
        <div className="logo" onClick={() => navigate("/")}>
          <i className="fas fa-graduation-cap"></i>
          <span>TestSystem</span>
        </div>

        <div className="nav-links">
          <Link to="/"><i className="fas fa-home"></i> Главная</Link>
          <Link to="/tests"><i className="fas fa-list"></i> Тесты</Link>
          {token && <Link to="/dashboard"><i className="fas fa-user"></i> Личный кабинет</Link>}
          {role === "admin" && <Link to="/admin"><i className="fas fa-tools"></i> Админка</Link>}
          {!token && (
            <>
              <button onClick={() => window.location.href = "http://localhost:8081/auth/yandex/login-web"}>
                <i className="fas fa-sign-in-alt"></i> Войти через Yandex
              </button>
              <button onClick={() => window.location.href = "http://localhost:8081/auth/github/login-web"}>
                <i className="fab fa-github"></i> Войти через GitHub
              </button>
            </>
          )}
          {token && (
            <button onClick={logout}>
              <i className="fas fa-sign-out-alt"></i> Выйти
            </button>
          )}
        </div>
      </nav>
    </header>
  );
}
